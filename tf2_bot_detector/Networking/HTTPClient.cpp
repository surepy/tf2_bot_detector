#include <mh/algorithm/multi_compare.hpp>
#include <mh/coroutine/task.hpp>
#include <mh/error/error_code_exception.hpp>
#include <mh/text/case_insensitive_string.hpp>

#include "Clock.h"
#include "HTTPClient.h"
#include "HTTPHelpers.h"
#include "Log.h"

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <curl/curl.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <exception>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace tf2_bot_detector;

namespace
{
	using http_clock_t = std::chrono::steady_clock;

	class HTTPTransportError final : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class CurlRuntime final
	{
	public:
		CurlRuntime()
		{
			const auto result = curl_global_init(CURL_GLOBAL_DEFAULT);
			if (result != CURLE_OK)
				throw HTTPTransportError(curl_easy_strerror(result));
		}
		~CurlRuntime() { curl_global_cleanup(); }
		CurlRuntime(const CurlRuntime&) = delete;
		CurlRuntime& operator=(const CurlRuntime&) = delete;
	};

	struct HTTPResponse
	{
		long status = 0;
		std::string body;
	};

	// Completions run here rather than on the I/O thread, so slow continuations
	// (JSON parsing, nested synchronous requests) never stall other transfers.
	class CompletionPool final
	{
	public:
		explicit CompletionPool(size_t threadCount)
		{
			try
			{
				for (size_t i = 0; i < threadCount; ++i)
					m_Threads.emplace_back([this] { Run(); });
			}
			catch (...)
			{
				Stop();
				throw;
			}
		}

		~CompletionPool() { Stop(); }

		void Post(std::coroutine_handle<> handle)
		{
			{
				std::lock_guard lock(m_Mutex);
				m_Queue.push_back(handle);
			}
			m_CV.notify_one();
		}

	private:
		void Stop()
		{
			{
				std::lock_guard lock(m_Mutex);
				m_Stopping = true;
			}
			m_CV.notify_all();
		}

		void Run()
		{
			while (true)
			{
				std::coroutine_handle<> handle;
				{
					std::unique_lock lock(m_Mutex);
					m_CV.wait(lock, [this] { return m_Stopping || !m_Queue.empty(); });
					if (m_Queue.empty())
						return; // Drain everything before exiting.
					handle = m_Queue.front();
					m_Queue.pop_front();
				}
				handle.resume();
			}
		}

		std::mutex m_Mutex;
		std::condition_variable m_CV;
		bool m_Stopping = false;
		std::deque<std::coroutine_handle<>> m_Queue;
		// Destroy first, joining threads before their queue and synchronization objects.
		std::vector<std::jthread> m_Threads;
	};

	// A scheduled transfer (or a plain delay when m_URL is empty). Lives in the
	// awaiting coroutine's frame, so it stays put until that coroutine resumes.
	struct Transfer
	{
		std::optional<URL> m_URL;
		std::string m_Host;
		http_clock_t::duration m_MinInterval{};
		std::function<void()> m_OnStart;
		std::coroutine_handle<> m_Coroutine;

		HTTPResponse m_Response;
		std::exception_ptr m_Error;
		std::array<char, CURL_ERROR_SIZE> m_ErrorBuffer{};
		std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> m_Easy{ nullptr, &curl_easy_cleanup };
	};

	// One thread drives every transfer through a curl multi handle: there is no
	// cap on concurrent requests, and idle connections are shared across them.
	// Per-host dispatch throttling is still enforced before a transfer starts.
	class HTTPEngine final
	{
	public:
		HTTPEngine() :
			m_Multi(curl_multi_init(), &curl_multi_cleanup),
			m_Completions(std::clamp<size_t>(std::thread::hardware_concurrency(), 4, 16))
		{
			if (!m_Multi)
				throw HTTPTransportError("Failed to initialize HTTP client");
			// Unlimited transfers, but keep a generous pool of idle connections for reuse.
			curl_multi_setopt(m_Multi.get(), CURLMOPT_MAXCONNECTS, 64L);
			m_Thread = std::jthread([this] { Run(); });
		}

		~HTTPEngine()
		{
			{
				std::lock_guard lock(m_Mutex);
				m_Stopping = true;
			}
			curl_multi_wakeup(m_Multi.get());
		}

		class Awaitable
		{
		public:
			Awaitable(HTTPEngine& engine, Transfer& transfer) : m_Engine(engine), m_Transfer(transfer) {}

			bool await_ready() const noexcept { return false; }
			void await_suspend(std::coroutine_handle<> handle)
			{
				m_Transfer.m_Coroutine = handle;
				m_Engine.Enqueue(m_Transfer, http_clock_t::now());
			}
			HTTPResponse await_resume()
			{
				if (m_Transfer.m_Error)
					std::rethrow_exception(m_Transfer.m_Error);
				return std::move(m_Transfer.m_Response);
			}

		private:
			HTTPEngine& m_Engine;
			Transfer& m_Transfer;
		};

		mh::task<HTTPResponse> Get(URL url, std::string host, http_clock_t::duration minInterval,
			std::function<void()> onStart)
		{
			Transfer transfer;
			transfer.m_URL = std::move(url);
			transfer.m_Host = std::move(host);
			transfer.m_MinInterval = minInterval;
			transfer.m_OnStart = std::move(onStart);
			Awaitable awaitable{ *this, transfer };
			co_return co_await awaitable;
		}

		mh::task<bool> DelayUntil(http_clock_t::time_point readyAt)
		{
			Transfer transfer;
			struct DelayAwaitable
			{
				HTTPEngine& m_Engine;
				Transfer& m_Transfer;
				http_clock_t::time_point m_ReadyAt;
				bool await_ready() const noexcept { return false; }
				void await_suspend(std::coroutine_handle<> handle)
				{
					m_Transfer.m_Coroutine = handle;
					m_Engine.Enqueue(m_Transfer, m_ReadyAt);
				}
				void await_resume() const noexcept {}
			} awaitable{ *this, transfer, readyAt };
			co_await awaitable;
			co_return true;
		}

	private:
		void Enqueue(Transfer& transfer, http_clock_t::time_point readyAt)
		{
			{
				std::lock_guard lock(m_Mutex);
				if (m_Stopping)
					throw std::runtime_error("HTTP engine is shutting down");
				m_Pending.emplace(std::make_pair(readyAt, m_NextTask++), &transfer);
			}
			curl_multi_wakeup(m_Multi.get());
		}

		// Removes due transfers from the queue, honoring per-host start intervals.
		std::vector<Transfer*> TakeReady(http_clock_t::time_point now)
		{
			std::vector<Transfer*> ready;
			while (!m_Pending.empty() && m_Pending.begin()->first.first <= now)
			{
				auto first = m_Pending.begin();
				Transfer& transfer = *first->second;
				if (!transfer.m_Host.empty())
				{
					auto& nextStart = m_NextHostStart[transfer.m_Host];
					if (nextStart > now)
					{
						auto deferred = m_Pending.extract(first);
						deferred.key().first = nextStart;
						m_Pending.insert(std::move(deferred));
						continue;
					}
					nextStart = now + transfer.m_MinInterval;
				}
				ready.push_back(&transfer);
				m_Pending.erase(first);
			}
			return ready;
		}

		void Start(Transfer& transfer)
		{
			if (!transfer.m_URL)
				return Complete(transfer); // Plain delay

			try
			{
				if (transfer.m_OnStart)
					transfer.m_OnStart();

				transfer.m_Easy.reset(curl_easy_init());
				if (!transfer.m_Easy)
					throw HTTPTransportError("Failed to initialize HTTP request");
				Configure(transfer);

				const auto result = curl_multi_add_handle(m_Multi.get(), transfer.m_Easy.get());
				if (result != CURLM_OK)
					throw HTTPTransportError(curl_multi_strerror(result));
			}
			catch (...)
			{
				transfer.m_Easy.reset();
				transfer.m_Error = std::current_exception();
				Complete(transfer);
			}
		}

		void Finish(CURL* easy, CURLcode result)
		{
			Transfer* transferPtr = nullptr;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &transferPtr);
			Transfer& transfer = *transferPtr;
			curl_multi_remove_handle(m_Multi.get(), easy);

			if (!transfer.m_Error)
			{
				if (result != CURLE_OK)
				{
					transfer.m_Error = std::make_exception_ptr(HTTPTransportError(fmt::format("HTTP GET failed: {}",
						transfer.m_ErrorBuffer[0] ? transfer.m_ErrorBuffer.data() : curl_easy_strerror(result))));
				}
				else if (const auto infoResult = curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &transfer.m_Response.status);
					infoResult != CURLE_OK)
				{
					transfer.m_Error = std::make_exception_ptr(HTTPTransportError(curl_easy_strerror(infoResult)));
				}
			}

			transfer.m_Easy.reset();
			Complete(transfer);
		}

		void Complete(Transfer& transfer)
		{
			m_Completions.Post(transfer.m_Coroutine);
		}

		void Run()
		{
			while (true)
			{
				std::vector<Transfer*> ready;
				auto nextReady = http_clock_t::time_point::max();
				{
					std::lock_guard lock(m_Mutex);
					if (m_Stopping)
						break;
					ready = TakeReady(http_clock_t::now());
					if (!m_Pending.empty())
						nextReady = m_Pending.begin()->first.first;
				}

				for (Transfer* transfer : ready)
					Start(*transfer);

				int running = 0;
				curl_multi_perform(m_Multi.get(), &running);

				int queued = 0;
				while (CURLMsg* message = curl_multi_info_read(m_Multi.get(), &queued))
				{
					if (message->msg == CURLMSG_DONE)
						Finish(message->easy_handle, message->data.result);
				}

				// Sleep until curl needs attention, the next queued transfer is due,
				// or Enqueue/the destructor wakes us.
				auto wait = 1000ms;
				if (long curlTimeout = -1; curl_multi_timeout(m_Multi.get(), &curlTimeout) == CURLM_OK && curlTimeout >= 0)
					wait = std::min(wait, std::chrono::milliseconds(curlTimeout));
				if (nextReady != http_clock_t::time_point::max())
				{
					wait = std::min(wait, std::chrono::ceil<std::chrono::milliseconds>(
						std::max<http_clock_t::duration>(nextReady - http_clock_t::now(), 0ms)));
				}
				if (wait > 0ms)
					curl_multi_poll(m_Multi.get(), nullptr, 0, static_cast<int>(wait.count()), nullptr);
			}

			// Shutting down: fail everything still outstanding.
			const auto Abort = [&](Transfer& transfer)
			{
				transfer.m_Error = std::make_exception_ptr(HTTPTransportError("HTTP engine is shutting down"));
				Complete(transfer);
			};

			for (CURL* easy : GetEasyHandles())
			{
				Transfer* transfer = nullptr;
				curl_easy_getinfo(easy, CURLINFO_PRIVATE, &transfer);
				curl_multi_remove_handle(m_Multi.get(), easy);
				transfer->m_Easy.reset();
				Abort(*transfer);
			}

			std::lock_guard lock(m_Mutex);
			for (auto& [key, transfer] : m_Pending)
				Abort(*transfer);
			m_Pending.clear();
		}

		std::vector<CURL*> GetEasyHandles()
		{
			std::vector<CURL*> result;
			if (CURL** handles = curl_multi_get_handles(m_Multi.get()))
			{
				for (CURL** it = handles; *it; ++it)
					result.push_back(*it);
				curl_free(handles);
			}
			return result;
		}

		static void Configure(Transfer& transfer)
		{
			CURL* easy = transfer.m_Easy.get();
			const auto SetOption = [easy](CURLoption option, auto value)
			{
				const auto result = curl_easy_setopt(easy, option, value);
				if (result != CURLE_OK)
					throw HTTPTransportError(curl_easy_strerror(result));
			};

			const auto& url = *transfer.m_URL;
			const auto address = url.GetSchemeHostPort() + (url.m_Path.empty() ? "/" : url.m_Path);

			SetOption(CURLOPT_URL, address.c_str());
			SetOption(CURLOPT_PRIVATE, static_cast<void*>(&transfer));
			SetOption(CURLOPT_CONNECTTIMEOUT_MS, 10'000L);
			SetOption(CURLOPT_TIMEOUT_MS, 30'000L);
			SetOption(CURLOPT_NOSIGNAL, 1L);
			SetOption(CURLOPT_FOLLOWLOCATION, 1L);
			SetOption(CURLOPT_MAXREDIRS, 20L);
			SetOption(CURLOPT_PROTOCOLS_STR, "http,https");
			SetOption(CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
			SetOption(CURLOPT_USERAGENT, "tf2-bot-detector");
			SetOption(CURLOPT_ACCEPT_ENCODING, "");
			SetOption(CURLOPT_SSLVERSION, static_cast<long>(CURL_SSLVERSION_TLSv1_2));
			SetOption(CURLOPT_SSL_VERIFYPEER, 1L);
			SetOption(CURLOPT_SSL_VERIFYHOST, 2L);
			// Preserve caller-encoded paths and queries, including dot segments.
			SetOption(CURLOPT_PATH_AS_IS, 1L);
			SetOption(CURLOPT_WRITEFUNCTION, &WriteBody);
			SetOption(CURLOPT_WRITEDATA, static_cast<void*>(&transfer));
			SetOption(CURLOPT_ERRORBUFFER, transfer.m_ErrorBuffer.data());
		}

		static size_t WriteBody(char* data, size_t size, size_t count, void* context) noexcept
		{
			auto& transfer = *static_cast<Transfer*>(context);
			const auto bytes = size * count;
			try
			{
				transfer.m_Response.body.append(data, bytes);
				return bytes;
			}
			catch (...)
			{
				// Exceptions must not cross libcurl's C callback boundary.
				transfer.m_Error = std::current_exception();
				return CURL_WRITEFUNC_ERROR;
			}
		}

		// Initialize before everything else; clean up after all handles are gone.
		CurlRuntime m_CurlRuntime;
		std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> m_Multi;
		std::mutex m_Mutex;
		bool m_Stopping = false;
		uint64_t m_NextTask = 0;
		std::map<std::pair<http_clock_t::time_point, uint64_t>, Transfer*> m_Pending;
		std::map<std::string, http_clock_t::time_point> m_NextHostStart;
		// Joins after the I/O thread, draining the aborts it posts during shutdown.
		CompletionPool m_Completions;
		// Destroy first, so the I/O loop stops before anything it touches.
		std::jthread m_Thread;
	};

	HTTPEngine& GetHTTPEngine()
	{
		static HTTPEngine engine;
		return engine;
	}

	class HTTPClientImpl final : public IHTTPClient
	{
	public:
		std::string GetString(const URL& url) const override;
		mh::task<std::string> GetStringAsync(URL url) const override;

		RequestCounts GetRequestCounts() const override;

	private:
		mutable std::atomic_uint32_t m_TotalRequestCount = 0;
		mutable std::atomic_uint32_t m_FailedRequestCount = 0;

		// This is a pretty stupid way of implementing this lol, but its easy
		struct RequestInProgressObj {};
		struct RequestQueuedObj {};
		const std::shared_ptr<RequestInProgressObj> m_InProgressRequestCount = std::make_shared<RequestInProgressObj>();
		const std::shared_ptr<RequestQueuedObj> m_QueuedRequestCount = std::make_shared<RequestQueuedObj>();
	};
}

std::string HTTPClientImpl::GetString(const URL& url) const
{
	auto task = GetStringAsync(url);
	task.wait();
	return std::move(task.get());
}

static duration_t GetMinRequestInterval(const URL& url)
{
	if (url.m_Host.ends_with("akamaihd.net") ||
		url.m_Host.ends_with("steamstatic.com"))
	{
		return 0ms;
	}
	else if (url.m_Host == "api.steampowered.com")
	{
		if (mh::case_insensitive_view(url.m_Path).find("/GetPlayerItems/") != url.m_Path.npos)
			return 1000ms; // This is a slow/heavily throttled api

		return 100ms;
	}
	else if (url.m_Host == "steamcommunity.com")
	{
		return 2000ms;
	}

	return 500ms;
}

mh::task<std::string> HTTPClientImpl::GetStringAsync(URL url) const try
{
	auto self = shared_from_this(); // Make sure we don't vanish
	auto& engine = GetHTTPEngine();
	std::shared_ptr<RequestInProgressObj> inProgressObj;
	std::shared_ptr<RequestQueuedObj> queuedObj;

	const auto SetThrottled = [&](bool throttled)
	{
		if (throttled)
		{
			queuedObj = m_QueuedRequestCount;
			inProgressObj.reset();
		}
		else
		{
			inProgressObj = m_InProgressRequestCount;
			queuedObj.reset();
		}
	};

	SetThrottled(false);

	int32_t retryCount = 0;
	while (true)
	{
		auto retryDelayTime = 10s;
		try
		{
			try // exceptions are fun and cool and not a code smell
			{
				uint32_t requestIndex = 0;

				const auto startTime = tfbd_clock_t::now();
				SetThrottled(true);
				// Keep the callable and task explicit: GCC 11 mishandles owning lambda
				// captures in a co_await operand, including URL's small strings.
				// Runs on the I/O thread when the transfer actually starts.
				auto onStart = [&SetThrottled, &requestIndex, this]
					{
						SetThrottled(false);
						requestIndex = ++m_TotalRequestCount;
					};
				auto responseTask = engine.Get(url, url.m_Host, GetMinRequestInterval(url), std::move(onStart));
				auto response = co_await responseTask;

				if (response.status >= 400 && response.status < 600)
					throw http_error((HTTPResponseCode)response.status, fmt::format("Failed to HTTP GET {}", url));

				const auto duration = tfbd_clock_t::now() - startTime;
				DebugLog("[{}ms] HTTP GET #{}: {}", std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), requestIndex, url);

				co_return std::move(response.body);
			}
			catch (...)
			{
				++m_FailedRequestCount;
				throw;
			}
		}
		catch (const http_error& e)
		{
			const auto PrintRetryWarning = [&](MH_SOURCE_LOCATION_AUTO(location))
			{
				DebugLogWarning(location, "HTTP {} on {}, retrying...", (int)e.code().value(), url);
			};

			if (e.code() == HTTPResponseCode::TooManyRequests && retryCount < 10)
			{
				// retry a fair number of times for http 429, some stuff is aggressively throttled
				PrintRetryWarning();
			}
			else if (e.code() == HTTPResponseCode::InternalServerError && retryCount < 5)
			{
				// retry a few times for http 500-class errors, might be tf2bd-util being broken
				PrintRetryWarning();
			}
			else if (mh::any_eq(e.code(), HTTPResponseCode::BadGateway, HTTPResponseCode::ServiceUnavailable) && retryCount < 20)
			{
				// give a good like 20 tries, since they are likely indicitive of an api being temporarily down
				PrintRetryWarning();
			}
			else
			{
				// give up
				throw; 
			}
		}
		catch (const HTTPTransportError&)
		{
			if (retryCount > 3)
			{
				// Give up after a few socket/timeout errors
				throw;
			}
		}

		// Wait and try again
		{
			SetThrottled(true);
			co_await engine.DelayUntil(http_clock_t::now() + retryDelayTime);
			SetThrottled(false);
		}
		retryCount++;
		DebugLogWarning("Retry #{} for {}", retryCount, url);
	}
}
catch (const http_error&)
{
	DebugLogException("{}", url);
	throw;
}
catch (...)
{
	LogException("{}", url);
	throw;
}

auto HTTPClientImpl::GetRequestCounts() const -> RequestCounts
{
	return RequestCounts
	{
		.m_Total = m_TotalRequestCount,
		.m_Failed = m_FailedRequestCount,
		.m_InProgress = static_cast<uint32_t>(m_InProgressRequestCount.use_count() - 1),
		.m_Throttled = static_cast<uint32_t>(m_QueuedRequestCount.use_count() - 1),
	};
}

std::shared_ptr<IHTTPClient> tf2_bot_detector::IHTTPClient::Create()
{
	return std::make_shared<HTTPClientImpl>();
}

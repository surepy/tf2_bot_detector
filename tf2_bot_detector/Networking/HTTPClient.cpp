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
#include <httplib.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace tf2_bot_detector;

namespace
{
	using ClientCache = std::map<std::string, std::unique_ptr<httplib::Client>>;
	using http_clock_t = std::chrono::steady_clock;

	class HTTPTransportError final : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	// Each worker owns its clients: keep-alive without sharing httplib's request mutex.
	// Timed jobs also keep throttling/retries independent of the application dispatcher.
	class HTTPWorkers final
	{
	public:
		// why 12? because it seemed like a good number (half of a 24-player server)
		static constexpr size_t WORKERS_MAX = 12;
	
		HTTPWorkers()
		{
			try
			{
				for (size_t i = 0; i < WORKERS_MAX; ++i)
					m_Threads.emplace_back([this] { Run(); });
			}
			catch (...)
			{
				Stop();
				throw;
			}
		}

		~HTTPWorkers() { Stop(); }

		template<typename TFunc>
		mh::task<std::invoke_result_t<TFunc, ClientCache&>> AddTask(TFunc func,
			http_clock_t::time_point readyAt = http_clock_t::now(), std::string host = {},
			http_clock_t::duration minInterval = {})
		{
			auto& clients = co_await ScheduledTask{ *this, readyAt, host, minInterval };
			co_return func(clients);
		}

		mh::task<bool> DelayUntil(http_clock_t::time_point readyAt)
		{
			return AddTask([](ClientCache&) { return true; }, readyAt);
		}

	private:
		struct ScheduledTask
		{
			HTTPWorkers& m_Workers;
			http_clock_t::time_point m_ReadyAt;
			std::string_view m_Host;
			http_clock_t::duration m_MinInterval;
			ClientCache* m_Clients = nullptr;

			bool await_ready() const noexcept { return false; }
			ClientCache& await_resume() const noexcept { return *m_Clients; }
			void await_suspend(std::coroutine_handle<> handle)
			{
				m_Workers.Enqueue(m_ReadyAt, std::string(m_Host), m_MinInterval, [this, handle](ClientCache& clients)
					{
						m_Clients = &clients;
						handle.resume();
					});
			}
		};

		struct WorkItem
		{
			std::string m_Host;
			http_clock_t::duration m_MinInterval;
			std::function<void(ClientCache&)> m_Task;
		};

		void Enqueue(http_clock_t::time_point readyAt, std::string host, http_clock_t::duration minInterval,
			std::function<void(ClientCache&)> task)
		{
			{
				std::lock_guard lock(m_Mutex);
				if (m_Stopping)
					throw std::runtime_error("HTTP workers are shutting down");
				m_Tasks.emplace(std::make_pair(readyAt, m_NextTask++),
					WorkItem{ std::move(host), minInterval, std::move(task) });
			}
			m_CV.notify_all();
		}

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
			ClientCache clients;
			while (true)
			{
				std::function<void(ClientCache&)> task;
				{
					std::unique_lock lock(m_Mutex);
					while (true)
					{
						if (m_Tasks.empty())
						{
							if (m_Stopping)
								return;
							m_CV.wait(lock);
						}
						else if (!m_Stopping && m_Tasks.begin()->first.first > http_clock_t::now())
						{
							const auto readyAt = m_Tasks.begin()->first.first;
							m_CV.wait_until(lock, readyAt);
						}
						else
						{
							auto first = m_Tasks.begin();
							if (!m_Stopping && !first->second.m_Host.empty())
							{
								const auto now = http_clock_t::now();
								auto& nextStart = m_NextHostStart[first->second.m_Host];
								if (nextStart > now)
								{
									// Enforce intervals at dispatch, including after all workers were busy.
									auto deferred = m_Tasks.extract(first);
									deferred.key().first = nextStart;
									m_Tasks.insert(std::move(deferred));
									continue;
								}
								nextStart = now + first->second.m_MinInterval;
							}
							task = std::move(first->second.m_Task);
							m_Tasks.erase(m_Tasks.begin());
							break;
						}
					}
				}
				task(clients);
			}
		}

		std::mutex m_Mutex;
		std::condition_variable m_CV;
		bool m_Stopping = false;
		uint64_t m_NextTask = 0;
		std::map<std::pair<http_clock_t::time_point, uint64_t>, WorkItem> m_Tasks;
		std::map<std::string, http_clock_t::time_point> m_NextHostStart;
		// Destroy first, joining workers before their queue and synchronization objects.
		std::vector<std::jthread> m_Threads;
	};

	HTTPWorkers& GetHTTPWorkers()
	{
		static HTTPWorkers workers;
		return workers;
	}

	httplib::Client& GetInnerClient(ClientCache& clients, const URL& url)
	{
		auto& client = clients[url.GetSchemeHostPort()];
		if (!client)
		{
			client = std::make_unique<httplib::Client>(url.GetSchemeHostPort());
			client->set_connection_timeout(10s);
			client->set_read_timeout(30s);
			client->set_write_timeout(30s);
			client->set_max_timeout(30s);
			client->set_keep_alive(true);
			client->set_follow_location(true);
			// Callers already percent-encode query parameters.
			client->set_path_encode(false);
			client->enable_server_certificate_verification(true);
		}
		return *client;
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
	auto& workers = GetHTTPWorkers();
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
				auto request = [url, &SetThrottled, &requestIndex, this](ClientCache& clients)
					{
						SetThrottled(false);
						requestIndex = ++m_TotalRequestCount;
						auto result = GetInnerClient(clients, url).Get(url.m_Path.empty() ? "/" : url.m_Path);
						if (!result)
							throw HTTPTransportError(fmt::format("HTTP GET failed: {}", httplib::to_string(result.error())));
						return std::move(*result);
					};
				auto responseTask = workers.AddTask(std::move(request), http_clock_t::now(),
					url.m_Host, GetMinRequestInterval(url));
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
			co_await workers.DelayUntil(http_clock_t::now() + retryDelayTime);
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

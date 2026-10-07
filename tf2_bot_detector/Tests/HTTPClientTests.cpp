#include "Networking/HTTPClient.h"
#include "Networking/HTTPHelpers.h"
#include "GlobalDispatcher.h"
#include "Log.h"

#include <catch2/catch_test_macros.hpp>
#include <httplib.h>
#include <zlib.h>

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <set>
#include <thread>

using namespace std::chrono_literals;
using namespace tf2_bot_detector;

// The standalone transport tests do not initialize application logging.
namespace tf2_bot_detector::detail::log_h
{
	void LogImplBase(const LogMessageColor&, LogSeverity, LogVisibility,
		const std::string_view&, const fmt::format_args&) {}
	void LogImplBase(const LogMessageColor&, LogSeverity, LogVisibility,
		const std::source_location&, const std::string_view&, const fmt::format_args&) {}
}
namespace tf2_bot_detector
{
	void LogException(const mh::source_location&, const std::exception_ptr&,
		LogSeverity, LogVisibility, const std::string_view&) {}
}

namespace
{
	template<typename TFunc>
	bool WaitUntil(TFunc predicate, std::chrono::seconds timeout = 2s)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (!predicate())
		{
			if (std::chrono::steady_clock::now() >= deadline)
				return false;
			std::this_thread::sleep_for(5ms);
		}
		return true;
	}

	std::string Gzip(const std::string& body)
	{
		z_stream stream{};
		REQUIRE(deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) == Z_OK);
		std::string compressed(compressBound(static_cast<uLong>(body.size())) + 32, '\0');
		stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(body.data()));
		stream.avail_in = static_cast<uInt>(body.size());
		stream.next_out = reinterpret_cast<Bytef*>(compressed.data());
		stream.avail_out = static_cast<uInt>(compressed.size());
		const auto status = deflate(&stream, Z_FINISH);
		compressed.resize(stream.total_out);
		deflateEnd(&stream);
		REQUIRE(status == Z_STREAM_END);
		return compressed;
	}

	class Server final
	{
	public:
		Server()
		{
			m_Server.new_task_queue = [] { return new httplib::ThreadPool(16); };
			m_Server.set_keep_alive_max_count(100);
			m_Server.set_keep_alive_timeout(30);
			m_Server.Get("/body", [](const httplib::Request&, httplib::Response& response)
				{ response.set_content("hello \xF0\x9F\x98\x80", "text/plain"); });
			m_Server.Get("/binary", [](const httplib::Request&, httplib::Response& response)
				{ response.set_content(std::string("\0\xff\x01\0", 4), "application/octet-stream"); });
			m_Server.Get("/gzip", [](const httplib::Request&, httplib::Response& response)
				{
					response.set_content(Gzip(std::string(4096, 'x')), "application/octet-stream");
					response.set_header("Content-Encoding", "gzip");
				});
			m_Server.Get("/redirect", [](const httplib::Request&, httplib::Response& response)
				{ response.set_redirect("/body"); });
			m_Server.Get("/query", [](const httplib::Request& request, httplib::Response& response)
				{ response.set_content(request.get_param_value("value"), "text/plain"); });
			m_Server.Get("/missing", [](const httplib::Request&, httplib::Response& response)
				{ response.status = 404; });
			m_Server.Get("/retry", [this](const httplib::Request&, httplib::Response& response)
				{
					response.status = ++m_RetryAttempts == 1 ? 500 : 200;
					response.set_content("recovered", "text/plain");
				});
			m_Server.Get("/transport-retry", [this](const httplib::Request&, httplib::Response& response)
				{
					// An invalid status line exercises transport-error recovery without DNS dependencies.
					response.status = ++m_TransportAttempts == 1 ? 9999 : 200;
					response.set_content("reconnected", "text/plain");
				});
			m_Server.Get("/gate", [this](const httplib::Request& request, httplib::Response& response)
				{
					std::unique_lock lock(m_Mutex);
					++m_Entered;
					++m_Active;
					m_MaxActive = std::max(m_MaxActive, m_Active);
					m_Ports.insert(request.remote_port);
					m_StartTimes.push_back(std::chrono::steady_clock::now());
					m_CV.notify_all();
					m_CV.wait_for(lock, 15s, [this] { return m_Released; });
					--m_Active;
					response.set_content("released", "text/plain");
				});
			m_Port = m_Server.bind_to_any_port("127.0.0.1");
			REQUIRE(m_Port > 0);
			m_Thread = std::jthread([this] { m_Server.listen_after_bind(); });
			REQUIRE(WaitUntil([this] { return m_Server.is_running(); }));
		}

		~Server()
		{
			Release();
			m_Server.stop();
		}

		URL GetURL(const std::string& path, const std::string& host = "127.0.0.1") const
		{
			URL url;
			url.m_Scheme = "http://";
			url.m_Host = host;
			url.m_Port = static_cast<unsigned int>(m_Port);
			url.m_Path = path;
			return url;
		}

		void Release()
		{
			std::lock_guard lock(m_Mutex);
			m_Released = true;
			m_CV.notify_all();
		}

		httplib::Server m_Server;
		std::mutex m_Mutex;
		std::condition_variable m_CV;
		int m_Entered = 0;
		int m_Active = 0;
		int m_MaxActive = 0;
		std::set<int> m_Ports;
		std::vector<std::chrono::steady_clock::time_point> m_StartTimes;
		bool m_Released = false;
		std::atomic_int m_RetryAttempts = 0;
		std::atomic_int m_TransportAttempts = 0;
		int m_Port = 0;
		std::jthread m_Thread;
	};

	mh::task<> FetchAndDispatch(std::shared_ptr<IHTTPClient> client, URL url,
		mh::dispatcher dispatcher, std::promise<std::thread::id>& completed)
	{
		co_await client->GetStringAsync(url);
		co_await dispatcher.co_dispatch();
		completed.set_value(std::this_thread::get_id());
	}
}

TEST_CASE("HTTP preserves bodies, redirects, and encoded queries", "[http]")
{
	Server server;
	auto client = IHTTPClient::Create();
	REQUIRE(client->GetString(server.GetURL("/body")) == "hello \xF0\x9F\x98\x80");
	REQUIRE(client->GetString(server.GetURL("/binary")) == std::string("\0\xff\x01\0", 4));
	REQUIRE(client->GetString(server.GetURL("/gzip")) == std::string(4096, 'x'));
	REQUIRE(client->GetString(server.GetURL("/redirect")) == "hello \xF0\x9F\x98\x80");
	REQUIRE(client->GetString(server.GetURL("/query?value=a%2Fb%20c%2Bd")) == "a/b c+d");
}

TEST_CASE("HTTP status errors retain their code and do not retry 404", "[http]")
{
	Server server;
	auto client = IHTTPClient::Create();
	try
	{
		client->GetString(server.GetURL("/missing"));
		FAIL("HTTP 404 should throw");
	}
	catch (const http_error& error)
	{
		REQUIRE(error.code() == HTTPResponseCode::NotFound);
	}
	REQUIRE(WaitUntil([&] { return client->GetRequestCounts().m_InProgress == 0; }));
	REQUIRE(client->GetRequestCounts().m_Total == 1);
	REQUIRE(client->GetRequestCounts().m_Failed == 1);
}

TEST_CASE("HTTP retries release workers and do not require a UI dispatcher", "[http]")
{
	Server server;
	auto client = IHTTPClient::Create();
	auto retry = client->GetStringAsync(server.GetURL("/retry"));
	auto transportRetry = client->GetStringAsync(server.GetURL("/transport-retry"));
	REQUIRE(WaitUntil([&] { return client->GetRequestCounts().m_Failed == 2; }));
	REQUIRE(retry.wait_for(1s) == std::future_status::timeout);
	REQUIRE(client->GetString(server.GetURL("/body")) == "hello \xF0\x9F\x98\x80");
	REQUIRE(retry.wait_for(15s) == std::future_status::ready);
	REQUIRE(retry.get() == "recovered");
	REQUIRE(transportRetry.wait_for(15s) == std::future_status::ready);
	REQUIRE(transportRetry.get() == "reconnected");
	REQUIRE(server.m_RetryAttempts == 2);
	REQUIRE(server.m_TransportAttempts == 2);
	REQUIRE(client->GetRequestCounts().m_Total == 5);
	REQUIRE(client->GetRequestCounts().m_Failed == 2);
}

TEST_CASE("HTTP overlaps same-host requests, caps operations at twelve, and reuses connections", "[http]")
{
	constexpr int MAX_ACTIVE_REQUESTS = 12;
	constexpr int TOTAL_REQUESTS = MAX_ACTIVE_REQUESTS + 2;
	Server server;
	auto client = IHTTPClient::Create();
	std::vector<mh::task<std::string>> tasks;
	const auto started = std::chrono::steady_clock::now();
	for (int i = 0; i < TOTAL_REQUESTS; ++i)
		tasks.push_back(client->GetStringAsync(server.GetURL("/gate", "localhost")));
	{
		std::unique_lock lock(server.m_Mutex);
		REQUIRE(server.m_CV.wait_for(lock, 9s, [&] { return server.m_Entered >= MAX_ACTIVE_REQUESTS; }));
		// Give all requests time to pass the existing 500ms throttle.
		server.m_CV.wait_until(lock, started + (TOTAL_REQUESTS + 1) * 500ms,
			[&] { return server.m_Entered > MAX_ACTIVE_REQUESTS; });
		REQUIRE(server.m_Entered == MAX_ACTIVE_REQUESTS);
		REQUIRE(server.m_MaxActive == MAX_ACTIVE_REQUESTS);
	}
	REQUIRE(client->GetRequestCounts().m_InProgress == MAX_ACTIVE_REQUESTS);
	REQUIRE(client->GetRequestCounts().m_Throttled == 2);
	server.Release();
	for (auto& task : tasks)
	{
		REQUIRE(task.wait_for(5s) == std::future_status::ready);
		REQUIRE(task.get() == "released");
	}
	{
		std::lock_guard lock(server.m_Mutex);
		REQUIRE(server.m_Entered == TOTAL_REQUESTS);
		REQUIRE(server.m_MaxActive == MAX_ACTIVE_REQUESTS);
		REQUIRE(server.m_Ports.size() == MAX_ACTIVE_REQUESTS);
		for (size_t i = 1; i < server.m_StartTimes.size(); ++i)
			REQUIRE(server.m_StartTimes[i] - server.m_StartTimes[i - 1] >= 450ms);
	}
	REQUIRE(WaitUntil([&]
		{
			const auto counts = client->GetRequestCounts();
			return counts.m_InProgress == 0 && counts.m_Throttled == 0;
		}));
	REQUIRE(client->GetRequestCounts().m_Total == TOTAL_REQUESTS);
	REQUIRE(client->GetRequestCounts().m_Failed == 0);
}

TEST_CASE("HTTP callers can still dispatch completion to the main thread", "[http]")
{
	Server server;
	mh::dispatcher dispatcher;
	std::promise<std::thread::id> completed;
	auto result = completed.get_future();
	auto task = FetchAndDispatch(IHTTPClient::Create(), server.GetURL("/body"), dispatcher, completed);
	REQUIRE(WaitUntil([&]
		{
			dispatcher.run();
			return result.wait_for(0s) == std::future_status::ready;
		}));
	REQUIRE(result.get() == std::this_thread::get_id());
	task.wait();
}

TEST_CASE("HTTP retains its client while an asynchronous request is pending", "[http]")
{
	Server server;
	auto client = IHTTPClient::Create();
	std::weak_ptr<IHTTPClient> lifetime = client;
	auto task = client->GetStringAsync(server.GetURL("/gate"));
	client.reset();
	REQUIRE_FALSE(lifetime.expired());
	{
		std::unique_lock lock(server.m_Mutex);
		REQUIRE(server.m_CV.wait_for(lock, 2s, [&] { return server.m_Entered == 1; }));
	}
	server.Release();
	REQUIRE(task.wait_for(5s) == std::future_status::ready);
	REQUIRE(task.get() == "released");
	REQUIRE(WaitUntil([&] { return lifetime.expired(); }));
}

// Opt in explicitly; normal tests use only loopback and need no internet access.
TEST_CASE("HTTP HTTPS smoke test against the release endpoint", "[.][https]")
{
	auto client = IHTTPClient::Create();
	const auto body = client->GetString("https://api.github.com/repos/surepy/tf2_bot_detector/releases");
	REQUIRE(nlohmann::json::parse(body).is_array());
}

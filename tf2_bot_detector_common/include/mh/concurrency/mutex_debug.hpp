#pragma once

#include <atomic>
#include <cassert>
#include <mutex>
#include <thread>

namespace mh
{
	// Drop-in replacement for mutex types that records data about where and when it was locked/unlocked.
	template<typename TMutex = std::mutex>
	class mutex_debug
	{
	public:
		~mutex_debug()
		{
			assert(m_OwningThreadID == std::thread::id{}); // Mutex being destructed while still owned
		}

		void lock()
		{
			m_Mutex.lock();
			lock_acquired();
		}

		bool try_lock()
		{
			const bool success = m_Mutex.try_lock();

			if (success)
				lock_acquired();

			return success;
		}

		void unlock()
		{
			lock_releasing();
			m_Mutex.unlock();
		}

#if _MSC_VER < 1938
		// TODO: this is optional
		using native_handle_type = typename TMutex::native_handle_type;
		native_handle_type native_handle() { return m_Mutex.native_handle(); }
#endif

	private:
		void lock_acquired()
		{
			assert(m_OwningThreadID == std::thread::id{});  // Attempted to lock while already locked
			m_OwningThreadID = std::this_thread::get_id();
		}
		void lock_releasing()
		{
			assert(m_OwningThreadID != std::thread::id{});  // Attempted to unlock while not already locked
			m_OwningThreadID = {};
		}

		TMutex m_Mutex;
		std::thread::id m_OwningThreadID;
	};
}

#pragma once

#if __has_include(<compare>)
#include <compare>
#endif

#include <memory>
#include <mutex>
#include <string>
#include <type_traits>

namespace mh
{
	namespace detail::status_hpp
	{
		template<typename TStatusObj>
		class status_base
		{
		public:
			using status_obj_type = TStatusObj;

			struct value_type
			{
#if __cpp_impl_three_way_comparison >= 201907
				auto operator<=>(const value_type&) const = default;
#endif

				const status_obj_type* operator->() const { return &m_Status; }
				status_obj_type* operator->() { return &m_Status; }

				status_obj_type m_Status{};
				std::string m_Message;
			};

			bool operator==(const status_base& other) const { return m_SharedData == other.m_SharedData; }

		protected:
			// Returns true if changed
			bool set(status_obj_type status, const std::string_view& msg = "")
			{
				std::lock_guard lock(m_SharedData->m_Mutex);

				value_type& value = m_SharedData->m_Value;

				bool changed = false;
				if (value.m_Status != status)
				{
					changed = true;
					value.m_Status = status;
				}

				if (value.m_Message != msg)
				{
					changed = true;
					value.m_Message = msg;
				}

				if (changed)
					m_SharedData->m_HasValue = true;

				return changed;
			}

			bool has_value() const
			{
				std::lock_guard lock(m_SharedData->m_Mutex);
				return m_SharedData->m_HasValue;
			}

			value_type get() const
			{
				std::lock_guard lock(m_SharedData->m_Mutex);
				return m_SharedData->m_Value;
			}

		private:
			struct SharedData
			{
				mutable std::mutex m_Mutex;
				value_type m_Value;
				bool m_HasValue = false;
			};
			std::shared_ptr<SharedData> m_SharedData = std::make_shared<SharedData>();

		protected:
			status_base() = default;
			explicit status_base(status_obj_type code, std::string msg = {})
			{
				set(code, std::move(msg));
			}
			status_base(const status_base& other) :
				m_SharedData(other.m_SharedData)
			{
			}

			std::shared_ptr<SharedData> get_shared_data() const { return m_SharedData; }
		};
	}

	template<typename TStatusObj>
	class status_source : public detail::status_hpp::status_base<TStatusObj>
	{
		using status_base = detail::status_hpp::status_base<TStatusObj>;
	public:
		using status_base::status_base;
		using status_base::set;

		auto get_reader() const;
	};

	template<typename TStatusObj>
	class status_reader : public detail::status_hpp::status_base<TStatusObj>
	{
		using status_base = detail::status_hpp::status_base<TStatusObj>;
	public:
		status_reader()
		{
		}
		status_reader(const status_source<TStatusObj>& source) :
			status_base(source)
		{
		}

		using status_base::get;
		using status_base::has_value;
	};

	template<typename TStatusObj>
	auto status_source<TStatusObj>::get_reader() const
	{
		return status_reader(*this);
	}
}

#pragma once

#include <imgui.h>
#include <mh/types/disable_copy_move.hpp>

namespace ImGuiDesktop
{
	namespace detail
	{
		template<typename T> struct StorageHelperBase;

		template<>
		struct StorageHelperBase<float> : mh::disable_copy_move
		{
		protected:
			static constexpr auto GET_FUNC = &ImGuiStorage::GetFloat;
			static constexpr auto SET_FUNC = &ImGuiStorage::SetFloat;
		};

		template<>
		struct StorageHelperBase<int> : mh::disable_copy_move
		{
		protected:
			static constexpr auto GET_FUNC = &ImGuiStorage::GetInt;
			static constexpr auto SET_FUNC = &ImGuiStorage::SetInt;
		};

		template<>
		struct StorageHelperBase<bool> : mh::disable_copy_move
		{
		protected:
			static constexpr auto GET_FUNC = &ImGuiStorage::GetBool;
			static constexpr auto SET_FUNC = &ImGuiStorage::SetBool;
		};
	}

	// *** THIS MUST BE STATICALLY ALLOCATED ***
	// TODO: Use platform APIs to make sure this isn't allocated on the stack
	template<typename T>
	struct Storage final : detail::StorageHelperBase<T>
	{
	private:
		using base_t = detail::StorageHelperBase<T>;

	public:
		struct Scope
		{
			[[nodiscard]] T Get(T defaultVal = {}) const
			{
				constexpr auto getter = base_t::GET_FUNC;
				T value = (m_Storage->*getter)(m_ID, defaultVal);
#ifdef _DEBUG
				m_LastValue = value;
#endif
				return value;
			}

			void Set(T value) const
			{
				constexpr auto setter = base_t::SET_FUNC;
				(m_Storage->*setter)(m_ID, value);

#ifdef _DEBUG
				m_LastValue = value;
#endif
			}

			operator T() const { return Get(); }
			const Scope& operator=(T value) const
			{
				Set(value);
				return *this;
			}

		private:
			friend struct Storage<T>;
			constexpr Scope(ImGuiStorage* storage, ImGuiID id) : m_Storage(storage), m_ID(id) {}

#ifdef _DEBUG
			mutable T m_LastValue{};
#endif

			ImGuiStorage* m_Storage{};
			ImGuiID m_ID{};
		};

		Scope Snapshot() const
		{
			return Scope(ImGui::GetStateStorage(), ImGui::GetID(this));
		}
	};
}

#pragma once

#include <memory>

namespace mh
{
	template<typename TClass, typename TMember>
	std::shared_ptr<TMember> shared_ptr_to_member(const std::shared_ptr<TClass>& ptr, [[maybe_unused]] TMember TClass::* memberVar)
	{
		// [[maybe_unused]] on memberVar because some compilers (clang 7/8/9/10/11) are stupid
		return std::shared_ptr<TMember>(ptr, &(ptr->memberVar));
	}
}

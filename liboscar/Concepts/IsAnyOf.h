#pragma once

#include <concepts>

namespace osc
{
	template<typename T, typename... U>
	concept IsAnyOf = (std::same_as<T, U> || ...);
}

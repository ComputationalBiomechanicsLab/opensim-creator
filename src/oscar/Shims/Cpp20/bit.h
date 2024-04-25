#pragma once

#include <concepts>
#include <cstring>
#include <limits>
#include <type_traits>

namespace osc::cpp20
{
    template<typename To, typename From>
    To bit_cast(From const& src) noexcept
    {
        static_assert(
            sizeof(To) == sizeof(From) &&
            std::is_trivially_copyable_v<From> &&
            std::is_trivially_copyable_v<To> &&
            std::is_trivially_constructible_v<To>
        );

        To dst;
        std::memcpy(&dst, &src, sizeof(To));
        return dst;
    }
}

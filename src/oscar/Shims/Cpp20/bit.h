#pragma once

#include <concepts>
#include <cstring>
#include <limits>
#include <type_traits>

namespace osc::cpp20
{
    template<std::unsigned_integral T>
    constexpr int bit_width(T x) noexcept
    {
        static_assert(!std::is_same_v<T, bool>);

        int rv = 0;
        while (x)
        {
            x >>= 1;
            ++rv;
        }
        return rv;
    }

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

#pragma once

#include <oscar/Maths/Vec.h>

#include <cstddef>

namespace osc
{
    template<size_t L, typename T>
    constexpr T const* value_ptr(Vec<L, T> const& v)
    {
        return v.data();
    }

    template<size_t L, typename T>
    constexpr T* value_ptr(Vec<L, T>& v)
    {
        return v.data();
    }
}

#pragma once

#include <oscar/Maths/Vec.h>

#include <cstddef>

namespace osc
{
    template<typename T>
    constexpr auto elementwise_map(Vec<2, T> const& v, T(*op)(T))
    {
        return Vec<2, T>{op(v.x), op(v.y)};
    }

    template<typename T>
    constexpr auto elementwise_map(Vec<2, T> const& v1, Vec<2, T> const& v2, T(*op)(T, T))
    {
        return Vec<2, T>{op(v1.x, v2.x), op(v1.y, v2.y)};
    }

    template<typename T>
    constexpr auto elementwise_map(Vec<3, T> const& v, T(*op)(T))
    {
        return Vec<3, T>{op(v.x), op(v.y), op(v.z)};
    }

    template<typename T>
    constexpr auto elementwise_map(Vec<3, T> const& v1, Vec<3, T> const& v2, T(*op)(T, T))
    {
        return Vec<3, T>{op(v1.x, v2.x), op(v1.y, v2.y), op(v1.z, v2.z)};
    }

    template<typename T>
    constexpr auto elementwise_map(Vec<4, T> const& v, T(*op)(T))
    {
        return Vec<4, T>{op(v.x), op(v.y), op(v.z), op(v.w)};
    }

    template<typename T>
    constexpr auto elementwise_map(Vec<4, T> const& v1, Vec<4, T> const& v2, T(*op)(T, T))
    {
        return Vec<4, T>{op(v1.x, v2.x), op(v1.y, v2.y), op(v1.z, v2.z), op(v1.w, v2.w)};
    }

    template<size_t L>
    constexpr bool all(Vec<L, bool> const& v)
    {
        for (size_t i = 0; i < L; ++i) {
            if (!v[i]) {
                return false;
            }
        }
        return true;
    }
}

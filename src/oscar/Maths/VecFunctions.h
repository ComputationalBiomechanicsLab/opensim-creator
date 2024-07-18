#pragma once

#include <oscar/Maths/Scalar.h>
#include <oscar/Maths/Vec.h>

#include <cstddef>

namespace osc
{
    template<size_t L, Scalar T>
    constexpr const T* value_ptr(const Vec<L, T>& vec)
    {
        return vec.data();
    }

    template<size_t L, Scalar T>
    constexpr T* value_ptr(Vec<L, T>& vec)
    {
        return vec.data();
    }
}

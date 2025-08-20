#pragma once

#include <liboscar/Maths/Scalar.h>
#include <liboscar/Maths/Vector.h>

#include <cstddef>

namespace osc
{
    template<size_t L, Scalar T>
    constexpr const T* value_ptr(const Vector<L, T>& vec)
    {
        return vec.data();
    }

    template<size_t L, Scalar T>
    constexpr T* value_ptr(Vector<L, T>& vec)
    {
        return vec.data();
    }
}

#pragma once

#include <liboscar/maths/scalar.h>
#include <liboscar/maths/vector.h>

#include <cstddef>

namespace osc
{
    template<Scalar T, size_t N>
    constexpr const T* value_ptr(const Vector<T, N>& vec)
    {
        return vec.data();
    }

    template<Scalar T, size_t N>
    constexpr T* value_ptr(Vector<T, N>& vec)
    {
        return vec.data();
    }
}

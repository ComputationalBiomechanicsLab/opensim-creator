#pragma once

#include <concepts>

namespace osc
{
    // Returns `value`, but with the value of the bit at `bit_index0` swapped
    // with the value of the bit at `bit_index1`.
    template<std::unsigned_integral T>
    constexpr T swap_single_bit(T value, int bit_index0, int bit_index1)
    {
        // Similar approach to the XOR swapping algorithm for integers:
        //
        // https://en.wikipedia.org/wiki/XOR_swap_algorithm

        const T bit0 = (value >> bit_index0) & T{1u};
        const T bit1 = (value >> bit_index1) & T{1u};

        T x = bit0 ^ bit1;
        x = (x << bit_index0) | (x << bit_index1);
        return x ^ value;
    }
}

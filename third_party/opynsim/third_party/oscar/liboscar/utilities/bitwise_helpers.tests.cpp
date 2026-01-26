#include "bitwise_helpers.h"

#include <gtest/gtest.h>

#include <cstdint>

using namespace osc;

TEST(swap_single_bit, returns_input_when_input_is_just_zeroes)
{
    uint64_t v{};
    for (int i = 0; i < static_cast<int>(8*sizeof(v)); ++i) {
        for (int j = 0; j < static_cast<int>(8*sizeof(v)); ++j) {
            ASSERT_EQ(swap_single_bit(v, i, j), v);
        }
    }
}

TEST(swap_single_bit, works_in_typical_case)
{
    static_assert(swap_single_bit(0b001000u, 3, 1) == 0b000010u);
}

TEST(swap_single_bit, returns_input_when_indices_are_equal)
{
    static_assert(swap_single_bit(0b00100u, 2, 2) == 0b00100u);
}

TEST(swap_single_bit, returns_input_when_bits_are_both_set)
{
    static_assert(swap_single_bit(0b001010u, 3, 1) == 0b001010u);
}

TEST(swap_single_bit, returns_input_when_bits_are_both_unset)
{
    static_assert(swap_single_bit(0b001010u, 2, 0) == 0b001010u);
}

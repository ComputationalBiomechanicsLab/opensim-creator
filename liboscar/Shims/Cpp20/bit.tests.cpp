#include "bit.h"

#include <gtest/gtest.h>

#include <bit>
#include <cstdint>

TEST(popcount, returns_expected_results)
{
    // this test is here because earlier versions of `oscar` used a shim
    // that this tested - the shim became unnecessary but the test stuck around

    static_assert(std::popcount(0b00000000u) == 0);
    static_assert(std::popcount(0b11111111u) == 8);
    static_assert(std::popcount(0b00011101u) == 4);
}

TEST(countrzero, returns_expected_results)
{
    // this test is here because earlier versions of `oscar` used a shim
    // that this tested - the shim became unnecessary but the test stuck around

    static_assert(std::countr_zero(static_cast<uint8_t>(0b00000000u)) == 8);
    static_assert(std::countr_zero(0b11111111u) == 0);
    static_assert(std::countr_zero(0b00011100u) == 2);
    static_assert(std::countr_zero(0b00011101u) == 0);
}

TEST(bitwidth, returns_expected_results)
{
    // this test is here because earlier versions of `oscar` used a shim
    // that this tested - the shim became unnecessary but the test stuck around

    static_assert(std::bit_width(0b0000u) == 0);
    static_assert(std::bit_width(0b0001u) == 1);
    static_assert(std::bit_width(0b0010u) == 2);
    static_assert(std::bit_width(0b0011u) == 2);
    static_assert(std::bit_width(0b0100u) == 3);
    static_assert(std::bit_width(0b0101u) == 3);
    static_assert(std::bit_width(0b0110u) == 3);
    static_assert(std::bit_width(0b0111u) == 3);

    static_assert(std::bit_width(2u) == 2);
}

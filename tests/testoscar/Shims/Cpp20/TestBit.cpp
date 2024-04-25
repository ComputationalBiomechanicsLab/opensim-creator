#include <oscar/Shims/Cpp20/bit.h>

#include <gtest/gtest.h>

#include <bit>
#include <cstdint>

namespace cpp20 = osc::cpp20;

TEST(popcount, ReturnsExpectedResults)
{
    static_assert(std::popcount(0b00000000u) == 0);
    static_assert(std::popcount(0b11111111u) == 8);
    static_assert(std::popcount(0b00011101u) == 4);
}

TEST(countrzero, ReturnsExpectedResults)
{
    static_assert(std::countr_zero(static_cast<uint8_t>(0b00000000u)) == 8);
    static_assert(std::countr_zero(0b11111111u) == 0);
    static_assert(std::countr_zero(0b00011100u) == 2);
    static_assert(std::countr_zero(0b00011101u) == 0);
}

TEST(bitwidth, ReturnsExpectedResults)
{
    static_assert(cpp20::bit_width(0b0000u) == 0);
    static_assert(cpp20::bit_width(0b0001u) == 1);
    static_assert(cpp20::bit_width(0b0010u) == 2);
    static_assert(cpp20::bit_width(0b0011u) == 2);
    static_assert(cpp20::bit_width(0b0100u) == 3);
    static_assert(cpp20::bit_width(0b0101u) == 3);
    static_assert(cpp20::bit_width(0b0110u) == 3);
    static_assert(cpp20::bit_width(0b0111u) == 3);

    static_assert(cpp20::bit_width(2u) == 2);
}

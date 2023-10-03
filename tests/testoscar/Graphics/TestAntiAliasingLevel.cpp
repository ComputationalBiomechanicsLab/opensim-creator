#include <oscar/Graphics/AntiAliasingLevel.hpp>

#include <gtest/gtest.h>

#include <string>
#include <sstream>

TEST(AntiAliasingLevel, DefaultCtorIsEquivalentTo1XLevel)
{
    static_assert(osc::AntiAliasingLevel{} == osc::AntiAliasingLevel{1});
}

TEST(AntiAliasingLevel, CtorWithZeroClampsToOne)
{
    static_assert(osc::AntiAliasingLevel{0} == osc::AntiAliasingLevel{1});
}

TEST(AntiAliasingLevel, CtorWithBelowZeroClampsToOne)
{
    static_assert(osc::AntiAliasingLevel{-1} == osc::AntiAliasingLevel{1});
}

TEST(AntiAliasingLevel, InvalidValuesClampedToNextLowerCorrectValue)
{
    static_assert(osc::AntiAliasingLevel{3} == osc::AntiAliasingLevel{2});
    static_assert(osc::AntiAliasingLevel{5} == osc::AntiAliasingLevel{4});
    static_assert(osc::AntiAliasingLevel{6} == osc::AntiAliasingLevel{4});
    static_assert(osc::AntiAliasingLevel{7} == osc::AntiAliasingLevel{4});
    static_assert(osc::AntiAliasingLevel{8} == osc::AntiAliasingLevel{8});
    static_assert(osc::AntiAliasingLevel{9} == osc::AntiAliasingLevel{8});
    static_assert(osc::AntiAliasingLevel{10} == osc::AntiAliasingLevel{8});
    static_assert(osc::AntiAliasingLevel{15} == osc::AntiAliasingLevel{8});
    static_assert(osc::AntiAliasingLevel{16} == osc::AntiAliasingLevel{16});
    static_assert(osc::AntiAliasingLevel{17} == osc::AntiAliasingLevel{16});
    static_assert(osc::AntiAliasingLevel{31} == osc::AntiAliasingLevel{16});
    static_assert(osc::AntiAliasingLevel{32} == osc::AntiAliasingLevel{32});
    static_assert(osc::AntiAliasingLevel{33} == osc::AntiAliasingLevel{32});
}

TEST(AntiAliasingLevel, IncrementGoesToTheNextLogicalAntiAliasingLevel)
{
    static_assert(++osc::AntiAliasingLevel{1} == osc::AntiAliasingLevel{2});
    static_assert(++osc::AntiAliasingLevel{2} == osc::AntiAliasingLevel{4});
    static_assert(++osc::AntiAliasingLevel{4} == osc::AntiAliasingLevel{8});
    static_assert(++osc::AntiAliasingLevel{8} == osc::AntiAliasingLevel{16});
}

TEST(AntiAliasingLevel, LessThanWorksAsExpected)
{
    static_assert(osc::AntiAliasingLevel{1} < osc::AntiAliasingLevel{2});
    static_assert(osc::AntiAliasingLevel{2} < osc::AntiAliasingLevel{4});
    static_assert(osc::AntiAliasingLevel{4} < osc::AntiAliasingLevel{8});
    static_assert(osc::AntiAliasingLevel{8} < osc::AntiAliasingLevel{16});
    static_assert(osc::AntiAliasingLevel{16} < osc::AntiAliasingLevel{32});
    static_assert(osc::AntiAliasingLevel{32} < osc::AntiAliasingLevel{64});
}

TEST(AntiAliasingLevel, GetU32ReturnsExpectedValues)
{
    static_assert(osc::AntiAliasingLevel{-1}.getU32() == 1u);
    static_assert(osc::AntiAliasingLevel{1}.getU32() == 1u);
    static_assert(osc::AntiAliasingLevel{2}.getU32() == 2u);
    static_assert(osc::AntiAliasingLevel{3}.getU32() == 2u);
    static_assert(osc::AntiAliasingLevel{4}.getU32() == 4u);
    static_assert(osc::AntiAliasingLevel{8}.getU32() == 8u);
}

TEST(AntiAliasingLevel, CanStreamToOutput)
{
    auto toString = [](osc::AntiAliasingLevel lvl) -> std::string
    {
        std::stringstream ss;
        ss << lvl;
        return std::move(ss).str();
    };

    ASSERT_EQ(toString(osc::AntiAliasingLevel{1}), "1x");
    ASSERT_EQ(toString(osc::AntiAliasingLevel{2}), "2x");
    ASSERT_EQ(toString(osc::AntiAliasingLevel{4}), "4x");
    ASSERT_EQ(toString(osc::AntiAliasingLevel{8}), "8x");
    ASSERT_EQ(toString(osc::AntiAliasingLevel{16}), "16x");
    ASSERT_EQ(toString(osc::AntiAliasingLevel{32}), "32x");
    ASSERT_EQ(toString(osc::AntiAliasingLevel{64}), "64x");
}

TEST(AntiAliasingLevel, MinReturns1X)
{
    static_assert(osc::AntiAliasingLevel::min() == osc::AntiAliasingLevel{1});
}

TEST(AntiAliasingLevel, NoneReturns1X)
{
    static_assert(osc::AntiAliasingLevel::none() == osc::AntiAliasingLevel{1});
}

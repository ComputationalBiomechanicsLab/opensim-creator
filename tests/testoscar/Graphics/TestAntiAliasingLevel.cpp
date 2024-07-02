#include <oscar/Graphics/AntiAliasingLevel.h>

#include <gtest/gtest.h>
#include <oscar/Utils/StringHelpers.h>

#include <sstream>
#include <string>

using namespace osc;

TEST(AntiAliasingLevel, default_constructor_returns_1x)
{
    static_assert(AntiAliasingLevel{} == AntiAliasingLevel{1});
}

TEST(AntiAliasingLevel, constructing_with_zero_clamps_to_1x)
{
    static_assert(AntiAliasingLevel{0} == AntiAliasingLevel{1});
}

TEST(AntiAliasingLevel, constructing_with_negative_number_clamps_to_1x)
{
    static_assert(AntiAliasingLevel{-1} == AntiAliasingLevel{1});
}

TEST(AntiAliasingLevel, constructing_with_invalid_values_clamps_to_next_lower_correct_value)
{
    static_assert(AntiAliasingLevel{3} == AntiAliasingLevel{2});
    static_assert(AntiAliasingLevel{5} == AntiAliasingLevel{4});
    static_assert(AntiAliasingLevel{6} == AntiAliasingLevel{4});
    static_assert(AntiAliasingLevel{7} == AntiAliasingLevel{4});
    static_assert(AntiAliasingLevel{8} == AntiAliasingLevel{8});
    static_assert(AntiAliasingLevel{9} == AntiAliasingLevel{8});
    static_assert(AntiAliasingLevel{10} == AntiAliasingLevel{8});
    static_assert(AntiAliasingLevel{15} == AntiAliasingLevel{8});
    static_assert(AntiAliasingLevel{16} == AntiAliasingLevel{16});
    static_assert(AntiAliasingLevel{17} == AntiAliasingLevel{16});
    static_assert(AntiAliasingLevel{31} == AntiAliasingLevel{16});
    static_assert(AntiAliasingLevel{32} == AntiAliasingLevel{32});
    static_assert(AntiAliasingLevel{33} == AntiAliasingLevel{32});
}

TEST(AntiAliasingLevel, increment_operator_multiplies_antialiasing_level_by_2)
{
    static_assert(++AntiAliasingLevel{1} == AntiAliasingLevel{2});
    static_assert(++AntiAliasingLevel{2} == AntiAliasingLevel{4});
    static_assert(++AntiAliasingLevel{4} == AntiAliasingLevel{8});
    static_assert(++AntiAliasingLevel{8} == AntiAliasingLevel{16});
}

TEST(AntiAliasingLevel, less_than_operator_behaves_as_expected)
{
    static_assert(AntiAliasingLevel{1} < AntiAliasingLevel{2});
    static_assert(AntiAliasingLevel{2} < AntiAliasingLevel{4});
    static_assert(AntiAliasingLevel{4} < AntiAliasingLevel{8});
    static_assert(AntiAliasingLevel{8} < AntiAliasingLevel{16});
    static_assert(AntiAliasingLevel{16} < AntiAliasingLevel{32});
    static_assert(AntiAliasingLevel{32} < AntiAliasingLevel{64});
}

TEST(AntiAliasingLevel, get_as_uint32_t_returns_expected_values)
{
    static_assert(AntiAliasingLevel{-1}.get_as<uint32_t>() == 1u);
    static_assert(AntiAliasingLevel{1}.get_as<uint32_t>() == 1u);
    static_assert(AntiAliasingLevel{2}.get_as<uint32_t>() == 2u);
    static_assert(AntiAliasingLevel{3}.get_as<uint32_t>() == 2u);
    static_assert(AntiAliasingLevel{4}.get_as<uint32_t>() == 4u);
    static_assert(AntiAliasingLevel{8}.get_as<uint32_t>() == 8u);
}

TEST(AntiAliasingLevel, operator_arrow_arrow_writes_equivalent_string_representation_to_ostream)
{
    ASSERT_EQ(stream_to_string(AntiAliasingLevel{1}), "1x");
    ASSERT_EQ(stream_to_string(AntiAliasingLevel{2}), "2x");
    ASSERT_EQ(stream_to_string(AntiAliasingLevel{4}), "4x");
    ASSERT_EQ(stream_to_string(AntiAliasingLevel{8}), "8x");
    ASSERT_EQ(stream_to_string(AntiAliasingLevel{16}), "16x");
    ASSERT_EQ(stream_to_string(AntiAliasingLevel{32}), "32x");
    ASSERT_EQ(stream_to_string(AntiAliasingLevel{64}), "64x");
}

TEST(AntiAliasingLevel, min_returns_1x)
{
    static_assert(AntiAliasingLevel::min() == AntiAliasingLevel{1});
}

TEST(AntiAliasingLevel, none_returns_1x)
{
    static_assert(AntiAliasingLevel::none() == AntiAliasingLevel{1});
}

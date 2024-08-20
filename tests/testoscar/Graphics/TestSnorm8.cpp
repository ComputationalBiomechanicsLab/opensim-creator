#include <oscar/Graphics/Snorm8.h>

#include <gtest/gtest.h>

#include <type_traits>

using namespace osc;

TEST(Snorm8, is_not_trivially_constructible)
{
    static_assert(not std::is_trivially_constructible_v<Snorm8>);
}

TEST(Snorm8, can_default_construct)
{
    [[maybe_unused]] Snorm8 default_constructed;
}

TEST(Snorm8, default_constructed_compares_equal_to_zero)
{
    static_assert(Snorm8{} == Snorm8{0});
}

TEST(Snorm8, raw_value_returns_provided_value)
{
    static_assert(Snorm8{5}.raw_value() == 5);
}

TEST(Snorm8, normalized_value_returns_normalized_value)
{
    // note: the fact that both `-128` and `-127` map onto
    // `-1.0f` is because that's how OpenGL handles the issue
    // of zero not being in the middle of `[-128, 127]`
    //
    // see: https://www.khronos.org/opengl/wiki/Normalized_Integer

    static_assert(Snorm8{-128}.normalized_value() == -1.0f);
    static_assert(Snorm8{-127}.normalized_value() == -1.0f);
    static_assert(Snorm8{0}.normalized_value() == 0.0f);
    static_assert(Snorm8{127}.normalized_value() == 1.0f);
}

TEST(Snorm8, constructed_from_float_quantizes_to_between_minus_127_and_127)
{
    static_assert(Snorm8{-1.0f} == Snorm8{-127});
    static_assert(Snorm8{ 0.0f} == Snorm8{0});
    static_assert(Snorm8{ 1.0f} == Snorm8{127});

    // saturates
    static_assert(Snorm8{-1.7f} == Snorm8{-127});
    static_assert(Snorm8{1.3f} == Snorm8{127});

    // NaN maps to -1
    static_assert(Snorm8{std::numeric_limits<float>::quiet_NaN()} == Snorm8{-1.0f});
}

TEST(Snorm8, implicit_conversion_to_float_is_equivalent_to_calling_normalized_value)
{
    static_assert(static_cast<float>(Snorm8{-3}) == Snorm8{-3}.normalized_value());
}

TEST(Snorm8, implicit_conversion_to_int8_is_equivalent_to_calling_raw_value)
{
    static_assert(static_cast<int8_t>(Snorm8{-47}) == Snorm8{-47}.raw_value());
}

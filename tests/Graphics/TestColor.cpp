#include "src/Graphics/Color.hpp"

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

TEST(Color, CanConstructFromRGBAFloats)
{
    osc::Color const color{1.0f, 0.0f, 0.0f, 0.0f};
}

TEST(Color, RGBAFloatConstructorIsConstexpr)
{
    // must compile
    osc::Color constexpr color{0.0f, 0.0f, 0.0f, 0.0f};
}

TEST(Color, CanBeExplicitlyConstructedFromVec4)
{
    osc::Color const color{glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}};
}

TEST(Color, CanBeImplicitlyConvertedToVec4)
{
    glm::vec4 constexpr v = osc::Color{0.0f, 0.0f, 1.0f, 0.0f};
}

TEST(Color, Vec4ConstructorIsConstexpr)
{
    // must compile
    osc::Color constexpr color{glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}};
}

TEST(Color, ToVec4ExplicitlyConvertsToVec4)
{
    osc::Color const c = {0.75, 0.75, 0.75, 1.0f};
    glm::vec4 const v = osc::ToVec4(c);

    ASSERT_EQ(v.x, c.r);
    ASSERT_EQ(v.y, c.g); 
    ASSERT_EQ(v.z, c.b);
    ASSERT_EQ(v.a, c.a);
}

TEST(Color, EqualityReturnsTrueForEquivalentColors)
{
    osc::Color const a = {1.0f, 0.0f, 1.0f, 0.5f};
    osc::Color const b = {1.0f, 0.0f, 1.0f, 0.5f};

    ASSERT_TRUE(a == b);
}

TEST(Color, EqualityReturnsFalseForInequivalentColors)
{
    osc::Color const a = {0.0f, 0.0f, 1.0f, 0.5f};
    osc::Color const b = {1.0f, 0.0f, 1.0f, 0.5f};

    ASSERT_FALSE(a == b);
}

TEST(Color, InequalityReturnsTrueForInequivalentColors)
{
    osc::Color const a = {0.0f, 0.0f, 1.0f, 0.5f};
    osc::Color const b = {1.0f, 0.0f, 1.0f, 0.5f};

    ASSERT_TRUE(a != b);
}

TEST(Color, InequalityReturnsFalseForEquivalentColors)
{
    osc::Color const a = {0.0f, 0.0f, 1.0f, 0.5f};
    osc::Color const b = {0.0f, 0.0f, 1.0f, 0.5f};

    ASSERT_FALSE(a != b);
}

TEST(Color, ToLinearReturnsLinearizedVersionOfColor)
{
    osc::Color const sRGBColor = {0.5f, 0.5f, 0.5f, 0.5f};
    osc::Color const linearColor = osc::ToLinear(sRGBColor);

    ASSERT_EQ(linearColor.r, std::pow(sRGBColor.r, 2.2f));
    ASSERT_EQ(linearColor.g, std::pow(sRGBColor.g, 2.2f));
    ASSERT_EQ(linearColor.b, std::pow(sRGBColor.b, 2.2f));
    ASSERT_EQ(linearColor.a, sRGBColor.a);
}

TEST(Color, ToSRGBReturnsColorWithGammaCurveApplied)
{
    osc::Color const linearColor = {0.25f, 0.25f, 0.25f, 0.6f};
    osc::Color const sRGBColor = osc::ToSRGB(linearColor);

    ASSERT_EQ(sRGBColor.r, std::pow(linearColor.r, 1.0f/2.2f));
    ASSERT_EQ(sRGBColor.g, std::pow(linearColor.g, 1.0f/2.2f));
    ASSERT_EQ(sRGBColor.b, std::pow(linearColor.b, 1.0f/2.2f));
    ASSERT_EQ(sRGBColor.a, linearColor.a);
}

TEST(Color, ToLinearFollowedByToSRGBEffectivelyReuturnsOriginalColor)
{
    osc::Color const originalColor = {0.1f, 0.1f, 0.1f, 0.5f};
    osc::Color const converted = osc::ToSRGB(osc::ToLinear(originalColor));

    float constexpr tolerance = 0.0001f;
    ASSERT_NEAR(originalColor.r, converted.r, tolerance);
    ASSERT_NEAR(originalColor.g, converted.g, tolerance);
    ASSERT_NEAR(originalColor.b, converted.b, tolerance);
    ASSERT_NEAR(originalColor.a, converted.a, tolerance);
}

TEST(Color, ToRgba32ReturnsRgba32VersionOfTheColor)
{
    osc::Color const color = {0.85f, 0.62f, 0.3f, 0.5f};
    osc::Rgba32 const expected
    {
        static_cast<uint8_t>(color.r * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.g * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.b * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.a * static_cast<float>(0xff)),
    };

    osc::Rgba32 const got = osc::ToRgba32(color);

    ASSERT_EQ(expected.r, got.r);
    ASSERT_EQ(expected.g, got.g);
    ASSERT_EQ(expected.b, got.b);
    ASSERT_EQ(expected.a, got.a);
}
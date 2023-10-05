#include <oscar/Graphics/Color.hpp>

#include <gtest/gtest.h>
#include <oscar/Utils/Cpp20Shims.hpp>

#include <type_traits>
#include <utility>

TEST(Color, CanConstructFromRGBAFloats)
{
    osc::Color const color{5.0f, 4.0f, 3.0f, 2.0f};
    ASSERT_EQ(color.r, 5.0f);
    ASSERT_EQ(color.g, 4.0f);
    ASSERT_EQ(color.b, 3.0f);
    ASSERT_EQ(color.a, 2.0f);
}

TEST(Color, RGBAFloatConstructorIsConstexpr)
{
    // must compile
    [[maybe_unused]] constexpr osc::Color color{0.0f, 0.0f, 0.0f, 0.0f};
}

TEST(Color, CanConstructFromRGBFloats)
{
    osc::Color const color{5.0f, 4.0f, 3.0f};
    ASSERT_EQ(color.r, 5.0f);
    ASSERT_EQ(color.g, 4.0f);
    ASSERT_EQ(color.b, 3.0f);

    ASSERT_EQ(color.a, 1.0f);  // default value when given RGB
}

TEST(Color, RGBFloatConstructorIsConstexpr)
{
    // must compile
    [[maybe_unused]] constexpr osc::Color color{0.0f, 0.0f, 0.0f};
}

TEST(Color, CanBeExplicitlyConstructedFromVec3)
{
    glm::vec3 const v = {0.25f, 0.387f, 0.1f};
    osc::Color const color{v};

    // ensure vec3 ctor creates a solid color with a == 1.0f
    ASSERT_EQ(color.r, v.x);
    ASSERT_EQ(color.g, v.y);
    ASSERT_EQ(color.b, v.z);
    ASSERT_EQ(color.a, 1.0f);
}

TEST(Color, CanBeExplicitlyConstructedFromVec4)
{
    [[maybe_unused]] osc::Color const color{glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}};
}

TEST(Color, CanBeImplicitlyConvertedToVec4)
{
    [[maybe_unused]] constexpr glm::vec4 v = osc::Color{0.0f, 0.0f, 1.0f, 0.0f};
}

TEST(Color, BracketOpertatorOnConstColorWorksAsExpected)
{
    osc::Color const color = {0.32f, 0.41f, 0.78f, 0.93f};

    ASSERT_EQ(color[0], color.r);
    ASSERT_EQ(color[1], color.g);
    ASSERT_EQ(color[2], color.b);
    ASSERT_EQ(color[3], color.a);
}

TEST(Color, Vec4ConstructorIsConstexpr)
{
    // must compile
    [[maybe_unused]] constexpr osc::Color color{glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}};
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

TEST(Color, CanMultiplyColors)
{
    osc::Color const a = {0.64f, 0.90f, 0.21f, 0.89f};
    osc::Color const b = {0.12f, 0.10f, 0.23f, 0.01f};

    osc::Color const rv = a * b;

    ASSERT_EQ(rv.r, a.r * b.r);
    ASSERT_EQ(rv.g, a.g * b.g);
    ASSERT_EQ(rv.b, a.b * b.b);
    ASSERT_EQ(rv.a, a.a * b.a);
}

TEST(Color, CanBeMutablyMultiplied)
{
    osc::Color const a = {0.64f, 0.90f, 0.21f, 0.89f};
    osc::Color const b = {0.12f, 0.10f, 0.23f, 0.01f};

    osc::Color rv = a;
    rv *= b;

    ASSERT_EQ(rv.r, a.r * b.r);
    ASSERT_EQ(rv.g, a.g * b.g);
    ASSERT_EQ(rv.b, a.b * b.b);
    ASSERT_EQ(rv.a, a.a * b.a);
}

TEST(Color, ToLinearReturnsLinearizedVersionOfOneColorChannel)
{
    float const sRGBColor = 0.02f;
    float const linearColor = osc::ToLinear(sRGBColor);

    // we don't test what the actual equation is, just that low
    // sRGB colors map to higher linear colors (i.e. they are
    // "stretched out" from the bottom of the curve)
    ASSERT_GT(sRGBColor, linearColor);
}

TEST(Color, ToSRGBReturnsSRGBVersionOfOneColorChannel)
{
    float const linearColor = 0.4f;
    float const sRGBColor = osc::ToSRGB(linearColor);

    // we don't test what the actual equation is, just that low-ish
    // linear colors are less than the equivalent sRGB color (because
    // sRGB will stretch lower colors out)
    ASSERT_LT(linearColor, sRGBColor);
}

TEST(Color, ToLinearReturnsLinearizedVersionOfColor)
{
    osc::Color const sRGBColor = {0.5f, 0.5f, 0.5f, 0.5f};
    osc::Color const linearColor = osc::ToLinear(sRGBColor);

    ASSERT_EQ(linearColor.r, osc::ToLinear(sRGBColor.r));
    ASSERT_EQ(linearColor.g, osc::ToLinear(sRGBColor.g));
    ASSERT_EQ(linearColor.b, osc::ToLinear(sRGBColor.b));
    ASSERT_EQ(linearColor.a, sRGBColor.a);
}

TEST(Color, ToSRGBReturnsColorWithGammaCurveApplied)
{
    osc::Color const linearColor = {0.25f, 0.25f, 0.25f, 0.6f};
    osc::Color const sRGBColor = osc::ToSRGB(linearColor);

    ASSERT_EQ(sRGBColor.r, osc::ToSRGB(linearColor.r));
    ASSERT_EQ(sRGBColor.g, osc::ToSRGB(linearColor.g));
    ASSERT_EQ(sRGBColor.b, osc::ToSRGB(linearColor.b));
    ASSERT_EQ(sRGBColor.a, linearColor.a);
}

TEST(Color, ToLinearFollowedByToSRGBEffectivelyReuturnsOriginalColor)
{
    osc::Color const originalColor = {0.1f, 0.1f, 0.1f, 0.5f};
    osc::Color const converted = osc::ToSRGB(osc::ToLinear(originalColor));

    constexpr float tolerance = 0.0001f;
    ASSERT_NEAR(originalColor.r, converted.r, tolerance);
    ASSERT_NEAR(originalColor.g, converted.g, tolerance);
    ASSERT_NEAR(originalColor.b, converted.b, tolerance);
    ASSERT_NEAR(originalColor.a, converted.a, tolerance);
}

TEST(Color, ToRgba32ReturnsRgba32VersionOfTheColor)
{
    osc::Color const color = {0.85f, 0.62f, 0.3f, 0.5f};
    osc::Color32 const expected
    {
        static_cast<uint8_t>(color.r * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.g * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.b * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.a * static_cast<float>(0xff)),
    };

    osc::Color32 const got = osc::ToColor32(color);

    ASSERT_EQ(expected.r, got.r);
    ASSERT_EQ(expected.g, got.g);
    ASSERT_EQ(expected.b, got.b);
    ASSERT_EQ(expected.a, got.a);
}

TEST(Color, ToColor32ClampsHDRValues)
{
    osc::Color const color = {1.5f, 0.0f, 2.0f, 1.0f};
    osc::Color32 const expected = {0xff, 0x00, 0xff, 0xff};
    ASSERT_EQ(osc::ToColor32(color), expected);
}

TEST(Color, ToColor32ClampsNegativeValues)
{
    osc::Color const color = {-1.0f, 0.0f, 1.0f, 1.0f};
    osc::Color32 const expected = {0x00, 0x00, 0xff, 0xff};
    ASSERT_EQ(osc::ToColor32(color), expected);
}

TEST(Color, CanGetBlackColor)
{
    ASSERT_EQ(osc::Color::black(), osc::Color(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST(Color, CanGetBlueColor)
{
    ASSERT_EQ(osc::Color::blue(), osc::Color(0.0f, 0.0f, 1.0f, 1.0f));
}

TEST(Color, CanGetClearColor)
{
    ASSERT_EQ(osc::Color::clear(), osc::Color(0.0f, 0.0f, 0.0f, 0.0f));
}

TEST(Color, CanGetGreenColor)
{
    ASSERT_EQ(osc::Color::green(), osc::Color(0.0f, 1.0f, 0.0f, 1.0f));
}

TEST(Color, CanGetRedColor)
{
    ASSERT_EQ(osc::Color::red(), osc::Color(1.0f, 0.0f, 0.0f, 1.0f));
}

TEST(Color, CanGetWhiteColor)
{
    ASSERT_EQ(osc::Color::white(), osc::Color(1.0f, 1.0f, 1.0f, 1.0f));
}

TEST(Color, CanGetYellowColor)
{
    ASSERT_EQ(osc::Color::yellow(), osc::Color(1.0f, 1.0f, 0.0f, 1.0f));
}

TEST(Color, ValuePtrConstVersionReturnsAddressOfColor)
{
    osc::Color const color = osc::Color::red();
    ASSERT_EQ(&color.r, osc::ValuePtr(color));
}

TEST(Color, ValuePtrMutatingVersionReturnsAddressOfColor)
{
    osc::Color color = osc::Color::red();
    ASSERT_EQ(&color.r, osc::ValuePtr(color));
}

TEST(Color, LerpWithZeroReturnsFirstColor)
{
    osc::Color const a = osc::Color::red();
    osc::Color const b = osc::Color::blue();

    ASSERT_EQ(osc::Lerp(a, b, 0.0f), a);
}

TEST(Color, LerpWithOneReturnsSecondColor)
{
    osc::Color const a = osc::Color::red();
    osc::Color const b = osc::Color::blue();

    ASSERT_EQ(osc::Lerp(a, b, 1.0f), b);
}

TEST(Color, LerpBelowZeroReturnsFirstColor)
{
    // tests that `t` is appropriately clamped

    osc::Color const a = osc::Color::red();
    osc::Color const b = osc::Color::blue();

    ASSERT_EQ(osc::Lerp(a, b, -1.0f), a);
}

TEST(Color, LerpAboveOneReturnsSecondColor)
{
    // tests that `t` is appropriately clamped

    osc::Color const a = osc::Color::red();
    osc::Color const b = osc::Color::blue();

    ASSERT_EQ(osc::Lerp(a, b, 2.0f), b);
}

TEST(Color, LerpBetweenTheTwoColorsReturnsExpectedResult)
{
    osc::Color const a = osc::Color::red();
    osc::Color const b = osc::Color::blue();
    float const t = 0.5f;
    float const tolerance = 0.0001f;

    osc::Color const rv = osc::Lerp(a, b, t);

    for (size_t i = 0; i < 4; ++i)
    {
        ASSERT_NEAR(rv[i], (1.0f-t)*a[i] + t*b[i], tolerance);
    }
}

TEST(Color, CanBeHashed)
{
    osc::Color const a = osc::Color::red();
    osc::Color const b = osc::Color::blue();

    ASSERT_NE(std::hash<osc::Color>{}(a), std::hash<osc::Color>{}(b));
}

TEST(Color, ToHtmlStringRGBAReturnsExpectedValues)
{
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::red()), "#ff0000ff");
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::green()), "#00ff00ff");
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::blue()), "#0000ffff");
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::black()), "#000000ff");
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::clear()), "#00000000");
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::white()), "#ffffffff");
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::yellow()), "#ffff00ff");
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::cyan()), "#00ffffff");
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color::magenta()), "#ff00ffff");

    // ... and HDR values are LDR clamped
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color(1.5f, 1.5f, 0.0f, 1.0f)), "#ffff00ff");

    // ... and negative values are clamped
    ASSERT_EQ(osc::ToHtmlStringRGBA(osc::Color(-1.0f, 0.0f, 0.0f, 1.0f)), "#000000ff");
}

TEST(Color, TryParseHtmlStringReturnsExpectedValues)
{
    // when caller specifies all channels
    ASSERT_EQ(osc::TryParseHtmlString("#ff0000ff"), osc::Color::red());
    ASSERT_EQ(osc::TryParseHtmlString("#00ff00ff"), osc::Color::green());
    ASSERT_EQ(osc::TryParseHtmlString("#0000ffff"), osc::Color::blue());
    ASSERT_EQ(osc::TryParseHtmlString("#000000ff"), osc::Color::black());
    ASSERT_EQ(osc::TryParseHtmlString("#ffff00ff"), osc::Color::yellow());
    ASSERT_EQ(osc::TryParseHtmlString("#00000000"), osc::Color::clear());

    // no colorspace conversion occurs on intermediate values (e.g. no sRGB-to-linear)
    ASSERT_EQ(osc::TryParseHtmlString("#110000ff"), osc::Color((1.0f*16.0f + 1.0f)/255.0f, 0.0f, 0.0f, 1.0f));

    // when caller specifies 3 channels, assume alpha == 1.0
    ASSERT_EQ(osc::TryParseHtmlString("#ff0000"), osc::Color::red());
    ASSERT_EQ(osc::TryParseHtmlString("#000000"), osc::Color::black());

    // unparseable input
    ASSERT_EQ(osc::TryParseHtmlString("not a color"), std::nullopt);
    ASSERT_EQ(osc::TryParseHtmlString(" #ff0000ff"), std::nullopt);  // caller handles whitespace
    ASSERT_EQ(osc::TryParseHtmlString("ff0000ff"), std::nullopt);  // caller must put the # prefix before the string
    ASSERT_EQ(osc::TryParseHtmlString("red"), std::nullopt);  // literal color strings (e.g. as in Unity) aren't supported (yet)
}

#include <oscar_document/Variant.hpp>

#include <glm/vec3.hpp>
#include <gtest/gtest.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>

#include <charconv>
#include <string>

TEST(Variant, CanExplicitlyContructFromBool)
{
    osc::doc::Variant vfalse{false};
    ASSERT_EQ(vfalse.toBool(), false);
    osc::doc::Variant vtrue{true};
    ASSERT_EQ(vtrue.toBool(), true);

    ASSERT_EQ(vtrue.getType(), osc::doc::VariantType::Bool);
}

TEST(Variant, CanExplicitlyConstructFromColor)
{
    osc::doc::Variant v{osc::Color::red()};
    ASSERT_EQ(v.toColor(), osc::Color::red());
    ASSERT_EQ(v.getType(), osc::doc::VariantType::Color);
}

TEST(Variant, CanExplicityConstructFromFloat)
{
    osc::doc::Variant v{1.0f};
    ASSERT_EQ(v.toFloat(), 1.0f);
    ASSERT_EQ(v.getType(), osc::doc::VariantType::Float);
}

TEST(Variant, CanExplicitlyConstructFromInt)
{
    osc::doc::Variant v{5};
    ASSERT_EQ(v.toInt(), 5);
    ASSERT_EQ(v.getType(), osc::doc::VariantType::Int);
}

TEST(Variant, CanExplicitlyConstructFromStringRValue)
{
    osc::doc::Variant v{std::string{"stringrval"}};
    ASSERT_EQ(v.toString(), "stringrval");
    ASSERT_EQ(v.getType(), osc::doc::VariantType::String);
}

TEST(Variant, CanExplicitlyConstructFromStringLiteral)
{
    osc::doc::Variant v{"cstringliteral"};
    ASSERT_EQ(v.toString(), "cstringliteral");
    ASSERT_EQ(v.getType(), osc::doc::VariantType::String);
}

TEST(Variant, CanExplicitlyConstructFromCStringView)
{
    osc::doc::Variant v{osc::CStringView{"cstringview"}};
    ASSERT_EQ(v.toString(), "cstringview");
    ASSERT_EQ(v.getType(), osc::doc::VariantType::String);
}

TEST(Variant, CanExplicitlyConstructFromVec3)
{
    osc::doc::Variant v{glm::vec3{1.0f, 2.0f, 3.0f}};
    ASSERT_EQ(v.toVec3(), glm::vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(v.getType(), osc::doc::VariantType::Vec3);
}

TEST(Variant, BoolValueToBoolReturnsExpectedBools)
{
    ASSERT_EQ(osc::doc::Variant(false).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(true).toBool(), true);
}

TEST(Variant, BoolValueToColorReturnsExpectedColors)
{
    ASSERT_EQ(osc::doc::Variant(false).toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant(true).toColor(), osc::Color::white());
}

TEST(Variant, BoolValueToFloatReturnsExpectedFloats)
{
    ASSERT_EQ(osc::doc::Variant(false).toFloat(), 0.0f);
    ASSERT_EQ(osc::doc::Variant(true).toFloat(), 1.0f);
}

TEST(Variant, BoolValueToIntReturnsExpectedInts)
{
    ASSERT_EQ(osc::doc::Variant(false).toInt(), 0);
    ASSERT_EQ(osc::doc::Variant(true).toInt(), 1);
}

TEST(Variant, BoolValueToStringReturnsExpectedStrings)
{
    osc::doc::Variant vfalse{false};
    ASSERT_EQ(vfalse.toString(), "false");
    osc::doc::Variant vtrue{true};
    ASSERT_EQ(vtrue.toString(), "true");
}

TEST(Variant, BoolValueToVec3ReturnsExpectedVectors)
{
    ASSERT_EQ(osc::doc::Variant(false).toVec3(), glm::vec3{});
    ASSERT_EQ(osc::doc::Variant(true).toVec3(), glm::vec3(1.0f, 1.0f, 1.0f));
}

TEST(Variant, ColorToBoolReturnsExpectedValues)
{
    ASSERT_EQ(osc::doc::Variant(osc::Color::black()).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(osc::Color::white()).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(osc::Color::magenta()).toBool(), true);
}

TEST(Variant, ColorToColorReturnsExpectedValues)
{
    ASSERT_EQ(osc::doc::Variant(osc::Color::black()).toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant(osc::Color::red()).toColor(), osc::Color::red());
    ASSERT_EQ(osc::doc::Variant(osc::Color::yellow()).toColor(), osc::Color::yellow());
}

TEST(Variant, ColorToFloatReturnsExpectedValues)
{
    ASSERT_EQ(osc::doc::Variant(osc::Color::black()).toFloat(), 0.0f);
    ASSERT_EQ(osc::doc::Variant(osc::Color::white()).toFloat(), 1.0f);
    ASSERT_EQ(osc::doc::Variant(osc::Color::blue()).toFloat(), 1.0f);
}

TEST(Variant, ColorToIntReturnsExpectedValues)
{
    ASSERT_EQ(osc::doc::Variant(osc::Color::black()).toInt(), 0);
    ASSERT_EQ(osc::doc::Variant(osc::Color::white()).toInt(), 1);
    ASSERT_EQ(osc::doc::Variant(osc::Color::cyan()).toInt(), 1);
}

TEST(Variant, ColorValueToStringReturnsSameAsToHtmlStringRGBA)
{
    auto const colors = osc::to_array(
    {
        osc::Color::red(),
        osc::Color::magenta(),
    });

    for (auto const& color : colors)
    {
        ASSERT_EQ(osc::doc::Variant(color).toString(), osc::ToHtmlStringRGBA(color));
    }
}

TEST(Variant, ColorValueToStringReturnsExpectedManualExamples)
{
    ASSERT_EQ(osc::doc::Variant{osc::Color::yellow()}.toString(), "#ffff00ff");
    ASSERT_EQ(osc::doc::Variant{osc::Color::magenta()}.toString(), "#ff00ffff");
}

TEST(Variant, ColorValueToVec3ReturnsFirst3Channels)
{
    ASSERT_EQ(osc::doc::Variant(osc::Color(1.0f, 2.0f, 3.0f)).toVec3(), glm::vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(osc::doc::Variant(osc::Color::red()).toVec3(), glm::vec3(1.0f, 0.0f, 0.0f));
}

TEST(Variant, FloatValueToBoolReturnsExpectedValues)
{
    ASSERT_EQ(osc::doc::Variant(0.0f).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(-0.5f).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(-1.0f).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(1.0f).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(0.75f).toBool(), true);
}

TEST(Variant, FloatValueToColorReturnsExpectedColor)
{
    for (float v : osc::to_array<float>({0.0f, 0.5f, 0.75, 1.0f}))
    {
        osc::Color const expected = {v, v, v};
        ASSERT_EQ(osc::doc::Variant(v).toColor(), expected);
    }
}

TEST(Variant, FloatValueToFloatReturnsInput)
{
    ASSERT_EQ(osc::doc::Variant(0.0f).toFloat(), 0.0f);
    ASSERT_EQ(osc::doc::Variant(0.12345f).toFloat(), 0.12345f);
    ASSERT_EQ(osc::doc::Variant(-0.54321f).toFloat(), 0.54321f);
}

TEST(Variant, FloatValueToIntReturnsCastedResult)
{
    for (float v : osc::to_array<float>({-0.5f, -0.123f, 0.0f, 1.0f, 1337.0f}))
    {
        int const expected = static_cast<int>(v);
        ASSERT_EQ(osc::doc::Variant(v).toInt(), expected);
    }
}

TEST(Variant, FloatValueToStringReturnsToStringedResult)
{
    for (float v : osc::to_array<float>({-5.35f, -2.0f, -1.0f, 0.0f, 0.123f, 18000.0f}))
    {
        std::string const expected = std::to_string(v);
        ASSERT_EQ(osc::doc::Variant(v).toString(), expected);
    }
}

TEST(Variant, FloatValueToVec3ReturnsVec3FilledWithFloat)
{
    for (float v : osc::to_array<float>({-20000.0f, -5.328f, -1.2f, 0.0f, 0.123f, 50.0f, 18000.0f}))
    {
        glm::vec3 const expected = {v, v, v};
        ASSERT_EQ(osc::doc::Variant(v).toVec3(), expected);
    }
}

TEST(Variant, IntValueToBoolReturnsExpectedResults)
{
    ASSERT_EQ(osc::doc::Variant(0).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(1).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(-1).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(234056).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(-12938).toBool(), true);
}

TEST(Variant, IntValueToColorReturnsBlackOrWhite)
{
    ASSERT_EQ(osc::doc::Variant(0).toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant(1).toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant(-1).toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant(-230244).toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant(100983).toColor(), osc::Color::white());
}

TEST(Variant, IntValueToFloatReturnsIntCastedToFloat)
{
    for (int v : osc::to_array<int>({-10000, -1000, -1, 0, 1, 17, 23000}))
    {
        float const expected = static_cast<float>(v);
        ASSERT_EQ(osc::doc::Variant(v).toFloat(), expected);
    }
}

TEST(Variant, IntValueToIntReturnsTheSuppliedInt)
{
    for (int v : osc::to_array<int>({ -123028, -2381, -32, -2, 0, 1, 1488, 5098}))
    {
        ASSERT_EQ(osc::doc::Variant(v).toInt(), v);
    }
}

TEST(Variant, IntValueToStringReturnsStringifiedInt)
{
    for (int v : osc::to_array<int>({ -121010, -13482, -1923, -123, -92, -7, 0, 1, 1294, 1209849}))
    {
        std::string const expected = std::to_string(v);
        ASSERT_EQ(osc::doc::Variant(v).toString(), expected);
    }
}

TEST(Variant, IntValueToVec3CastsValueToFloatThenPlacesInAllSlots)
{
    for (int v : osc::to_array<int>({ -12193, -1212, -738, -12, -1, 0, 1, 18, 1294, 1209849}))
    {
        float const vf = static_cast<float>(v);
        glm::vec3 const expected = {vf, vf, vf};
        ASSERT_EQ(osc::doc::Variant(v).toVec3(), expected);
    }
}

TEST(Variant, StringValueToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(osc::doc::Variant("false").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("FALSE").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("False").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("FaLsE").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("0").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("").toBool(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(osc::doc::Variant("true").toBool(), true);
    ASSERT_EQ(osc::doc::Variant("non-empty string").toBool(), true);
    ASSERT_EQ(osc::doc::Variant(" ").toBool(), true);
}

TEST(Variant, StringValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(osc::doc::Variant{"#ff0000ff"}.toColor(), osc::Color::red());
    ASSERT_EQ(osc::doc::Variant{"#00ff00ff"}.toColor(), osc::Color::green());
    ASSERT_EQ(osc::doc::Variant{"#ffffffff"}.toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant{"#00000000"}.toColor(), osc::Color::clear());
    ASSERT_EQ(osc::doc::Variant{"#000000ff"}.toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant{"#000000FF"}.toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant{"#123456ae"}.toColor(), *osc::TryParseHtmlString("#123456ae"));
}

TEST(Variant, StringValueColorReturnsWhiteIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(osc::doc::Variant{"not a color"}.toColor(), osc::Color::white());
}

// TODO: string --> float
// TODO: string --> int
// TODO: string --> string
// TODO: string --> vec3

// TODO: vec3 --> bool
// TODO: vec3 --> color
// TODO: vec3 --> float
// TODO: vec3 --> int
// TODO: vec3 --> string
// TODO: vec3 --> vec3

// TODO: comparison operations
// TODO: hashing

#include <oscar/Maths/CoordinateAxis.h>

#include <gtest/gtest.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/StringHelpers.h>

#include <array>
#include <concepts>
#include <optional>
#include <string_view>

using namespace osc;

TEST(CoordinateAxis, IsRegular)
{
    static_assert(std::regular<CoordinateAxis>);
}

TEST(CoordinateAxis, WhenDefaultConstructedIsEqualToXAxis)
{
    static_assert(CoordinateAxis{} == CoordinateAxis::x());
}

TEST(CoordinateAxis, XYZAreEqualToThemselves)
{
    static_assert(CoordinateAxis::x() == CoordinateAxis::x());
    static_assert(CoordinateAxis::y() == CoordinateAxis::y());
    static_assert(CoordinateAxis::z() == CoordinateAxis::z());
}

TEST(CoordinateAxis, AxesArentEqualToEachover)
{
    static_assert(CoordinateAxis::x() != CoordinateAxis::y());
    static_assert(CoordinateAxis::x() != CoordinateAxis::z());
    static_assert(CoordinateAxis::y() != CoordinateAxis::z());
}

TEST(CoordinateAxis, HasExpectedAxisWhenConstructedFromInteger)
{
    static_assert(CoordinateAxis{0} == CoordinateAxis::x());
    static_assert(CoordinateAxis{1} == CoordinateAxis::y());
    static_assert(CoordinateAxis{2} == CoordinateAxis::z());
}

TEST(CoordinateAxis, AreTotallyOrdered)
{
    static_assert(CoordinateAxis::x() < CoordinateAxis::y());
    static_assert(CoordinateAxis::y() < CoordinateAxis::z());
    static_assert(CoordinateAxis::x() < CoordinateAxis::z());  // transitive
}

TEST(CoordinateAxis, XIndexReturnsExpectedResults)
{
    static_assert(CoordinateAxis::x().index() == 0);
    static_assert(CoordinateAxis::y().index() == 1);
    static_assert(CoordinateAxis::z().index() == 2);
}

TEST(CoordinateAxis, NextWorksAsExpected)
{
    static_assert(CoordinateAxis::x().next() == CoordinateAxis::y());
    static_assert(CoordinateAxis::y().next() == CoordinateAxis::z());
    static_assert(CoordinateAxis::z().next() == CoordinateAxis::x());
}

TEST(CoordinateAxis, PrevWorksAsExpected)
{
    static_assert(CoordinateAxis::x().previous() == CoordinateAxis::z());
    static_assert(CoordinateAxis::y().previous() == CoordinateAxis::x());
    static_assert(CoordinateAxis::z().previous() == CoordinateAxis::y());
}

TEST(CoordinateAxis, StreamingOutputWorksAsExpected)
{
    ASSERT_EQ(stream_to_string(CoordinateAxis::x()), "x");
    ASSERT_EQ(stream_to_string(CoordinateAxis::y()), "y");
    ASSERT_EQ(stream_to_string(CoordinateAxis::z()), "z");
}

// parsing test cases
namespace
{
    struct ParsingTestCase final {
        std::string_view input;
        std::optional<CoordinateAxis> expected;
    };

    constexpr auto c_ParsingTestCases = std::to_array<ParsingTestCase>({
        // blank/value-initialized
        {"", std::nullopt},
        {{}, std::nullopt},

        // normal cases
        {"x", CoordinateAxis::x()},
        {"X", CoordinateAxis::x()},
        {"y", CoordinateAxis::y()},
        {"Y", CoordinateAxis::y()},
        {"z", CoordinateAxis::z()},
        {"Z", CoordinateAxis::z()},

        // signed cases should fail: callers should use `CoordinateDirection`
        {"+", std::nullopt},
        {"-", std::nullopt},
        {"+x", std::nullopt},
        {"+X", std::nullopt},
        {"-x", std::nullopt},
        {"-X", std::nullopt},
        {"+y", std::nullopt},
        {"+Y", std::nullopt},
        {"-y", std::nullopt},
        {"-Y", std::nullopt},
        {"+z", std::nullopt},
        {"+Z", std::nullopt},
        {"-z", std::nullopt},
        {"-Z", std::nullopt},

        // obviously invalid cases
        {"xenomorph", std::nullopt},
        {"yelp", std::nullopt},
        {"zodiac", std::nullopt},

        // padding is invalid (the caller should remove it)
        {" x", std::nullopt},
        {"x ", std::nullopt},
    });

    class CoordinateAxisParsingTestFixture : public testing::TestWithParam<ParsingTestCase> {};
}

INSTANTIATE_TEST_SUITE_P(
    CoordinateAxisParsingTest,
    CoordinateAxisParsingTestFixture,
    testing::ValuesIn(c_ParsingTestCases)
);

TEST_P(CoordinateAxisParsingTestFixture, Check)
{
    ParsingTestCase const tc =  GetParam();
    ASSERT_EQ(CoordinateAxis::try_parse(tc.input), tc.expected) << "input = " << tc.input;
}

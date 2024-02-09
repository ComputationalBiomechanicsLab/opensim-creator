#include <OpenSimCreator/Documents/Frames/FrameAxis.h>

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>

using osc::frames::AreOrthogonal;
using osc::frames::FrameAxis;
using osc::frames::TryParseAsFrameAxis;

namespace
{
    std::string StreamToString(FrameAxis fa)
    {
        std::stringstream ss;
        ss << fa;
        return std::move(ss).str();
    }
}

TEST(FrameAxis, TryParseAsFrameAxisReturnsNulloptForBlankInput)
{
    ASSERT_EQ(TryParseAsFrameAxis(""), std::nullopt);
}

TEST(FrameAxis, TryParseAsFrameAxisReturnsNulloptForValueInitializedInput)
{
    ASSERT_EQ(TryParseAsFrameAxis({}), std::nullopt);
}

TEST(FrameAxis, TryParseAsFrameAxisParsesXAsPlusX)
{
    ASSERT_EQ(TryParseAsFrameAxis("x"), FrameAxis::PlusX);
    ASSERT_EQ(TryParseAsFrameAxis("X"), FrameAxis::PlusX);  // case-insensitive
}

TEST(FrameAxis, TryParseAsFrameAxisParsesPlusXAsPlusX)
{
    ASSERT_EQ(TryParseAsFrameAxis("+x"), FrameAxis::PlusX);
    ASSERT_EQ(TryParseAsFrameAxis("+X"), FrameAxis::PlusX);
}

TEST(FrameAxis, TryParseFrameAxisParsesMinusXAsMinusX)
{
    ASSERT_EQ(TryParseAsFrameAxis("-x"), FrameAxis::MinusX);
    ASSERT_EQ(TryParseAsFrameAxis("-X"), FrameAxis::MinusX);  // case-insensitive
}

TEST(FrameAxis, TryParseAsFrameAxisBehavesSameForYAsX)
{
    ASSERT_EQ(TryParseAsFrameAxis("y"), FrameAxis::PlusY);
    ASSERT_EQ(TryParseAsFrameAxis("Y"), FrameAxis::PlusY);
    ASSERT_EQ(TryParseAsFrameAxis("+y"), FrameAxis::PlusY);
    ASSERT_EQ(TryParseAsFrameAxis("+Y"), FrameAxis::PlusY);
    ASSERT_EQ(TryParseAsFrameAxis("-y"), FrameAxis::MinusY);
    ASSERT_EQ(TryParseAsFrameAxis("-Y"), FrameAxis::MinusY);
}

TEST(FrameAxis, TryParseAsFrameAxisBehavesSameForZAsX)
{
    ASSERT_EQ(TryParseAsFrameAxis("z"), FrameAxis::PlusZ);
    ASSERT_EQ(TryParseAsFrameAxis("Z"), FrameAxis::PlusZ);
    ASSERT_EQ(TryParseAsFrameAxis("+z"), FrameAxis::PlusZ);
    ASSERT_EQ(TryParseAsFrameAxis("+Z"), FrameAxis::PlusZ);
    ASSERT_EQ(TryParseAsFrameAxis("-z"), FrameAxis::MinusZ);
    ASSERT_EQ(TryParseAsFrameAxis("-Z"), FrameAxis::MinusZ);
}

TEST(FrameAxis, TryParseAsFrameReturnsNulloptForJustPlusOrMinus)
{
    ASSERT_EQ(TryParseAsFrameAxis("+"), std::nullopt);
    ASSERT_EQ(TryParseAsFrameAxis("-"), std::nullopt);
}

TEST(FrameAxis, RejectsAdditionalInput)
{
    ASSERT_EQ(TryParseAsFrameAxis("xenomorph"), std::nullopt);
    ASSERT_EQ(TryParseAsFrameAxis("yelp"), std::nullopt);
    ASSERT_EQ(TryParseAsFrameAxis("zodiac"), std::nullopt);
    ASSERT_EQ(TryParseAsFrameAxis("-xe"), std::nullopt);
    // etc.
}

TEST(FrameAxis, AreOrthogonalBehavesAsExpected)
{
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusX, FrameAxis::PlusY));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusX, FrameAxis::MinusY));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusX, FrameAxis::PlusZ));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusX, FrameAxis::MinusZ));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusX, FrameAxis::PlusY));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusX, FrameAxis::MinusY));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusX, FrameAxis::PlusZ));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusX, FrameAxis::MinusZ));

    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusY, FrameAxis::PlusX));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusY, FrameAxis::MinusX));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusY, FrameAxis::PlusZ));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusY, FrameAxis::MinusZ));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusY, FrameAxis::PlusX));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusY, FrameAxis::MinusX));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusY, FrameAxis::PlusZ));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusY, FrameAxis::MinusZ));

    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusZ, FrameAxis::PlusX));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusZ, FrameAxis::MinusX));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusZ, FrameAxis::PlusY));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::PlusZ, FrameAxis::MinusY));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusZ, FrameAxis::PlusX));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusZ, FrameAxis::MinusX));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusZ, FrameAxis::PlusY));
    ASSERT_TRUE(AreOrthogonal(FrameAxis::MinusZ, FrameAxis::MinusY));

    ASSERT_FALSE(AreOrthogonal(FrameAxis::PlusX, FrameAxis::PlusX));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::PlusX, FrameAxis::MinusX));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::MinusX, FrameAxis::PlusX));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::MinusX, FrameAxis::MinusX));

    ASSERT_FALSE(AreOrthogonal(FrameAxis::PlusY, FrameAxis::PlusY));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::PlusY, FrameAxis::MinusY));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::MinusY, FrameAxis::PlusY));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::MinusY, FrameAxis::MinusY));

    ASSERT_FALSE(AreOrthogonal(FrameAxis::PlusZ, FrameAxis::PlusZ));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::PlusZ, FrameAxis::MinusZ));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::MinusZ, FrameAxis::PlusZ));
    ASSERT_FALSE(AreOrthogonal(FrameAxis::MinusZ, FrameAxis::MinusZ));
}

TEST(FrameAxis, StreamToStringBehavesAsExpected)
{
    ASSERT_EQ(StreamToString(FrameAxis::PlusX), "x");
    ASSERT_EQ(StreamToString(FrameAxis::PlusY), "y");
    ASSERT_EQ(StreamToString(FrameAxis::PlusZ), "z");
    ASSERT_EQ(StreamToString(FrameAxis::MinusX), "-x");
    ASSERT_EQ(StreamToString(FrameAxis::MinusY), "-y");
    ASSERT_EQ(StreamToString(FrameAxis::MinusZ), "-z");
}

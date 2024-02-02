#include <oscar/Utils/NullStreambuf.hpp>

#include <gtest/gtest.h>

#include <iosfwd>

using osc::NullStreambuf;

TEST(NullStreambuf, CanBeConstructed)
{
    ASSERT_NO_THROW({ NullStreambuf{}; });
}

TEST(NullStreambuf, CanBeWrappedIntoAnOStream)
{
    NullStreambuf buf;
    ASSERT_NO_THROW({ std::ostream o{&buf}; });
}

TEST(NullStreambuf, CanWriteToNullStreambufViaOstream)
{
    NullStreambuf buf;
    std::ostream o{&buf};

    ASSERT_NO_THROW({ o << "some content"; });
}

TEST(NullStreambuf, NumCharsWrittenReturnsZeroOnInitialConstruction)
{
    ASSERT_EQ(NullStreambuf{}.numCharsWritten(), 0);
}

TEST(NullStreambuf, NumCharsWrittenUpdatesAfterWriting)
{
    NullStreambuf buf;
    std::ostream{&buf} << "12345";
    ASSERT_EQ(buf.numCharsWritten(), 5);
}

TEST(NullStreambuf, WasWrittenToReturnsFalseInitially)
{
    ASSERT_FALSE(NullStreambuf{}.wasWrittenTo());
}

TEST(NullStreambuf, WasWrittenToReturnsTrueAfterBeingWrittenTo)
{
    NullStreambuf buf;
    std::ostream{&buf} << "12345";
    ASSERT_TRUE(buf.wasWrittenTo());
}

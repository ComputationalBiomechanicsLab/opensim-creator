#include <oscar/Utils/NullStreambuf.h>

#include <gtest/gtest.h>

#include <iosfwd>

using namespace osc;

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
    ASSERT_EQ(NullStreambuf{}.num_chars_written(), 0);
}

TEST(NullStreambuf, NumCharsWrittenUpdatesAfterWriting)
{
    NullStreambuf buf;
    std::ostream{&buf} << "12345";
    ASSERT_EQ(buf.num_chars_written(), 5);
}

TEST(NullStreambuf, WasWrittenToReturnsFalseInitially)
{
    ASSERT_FALSE(NullStreambuf{}.was_written_to());
}

TEST(NullStreambuf, WasWrittenToReturnsTrueAfterBeingWrittenTo)
{
    NullStreambuf buf;
    std::ostream{&buf} << "12345";
    ASSERT_TRUE(buf.was_written_to());
}

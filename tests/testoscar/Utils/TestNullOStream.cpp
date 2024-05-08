#include <oscar/Utils/NullOStream.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(NullOStream, CanConstruct)
{
    ASSERT_NO_THROW({ NullOStream{}; });
}

TEST(NullOStream, NumCharsWrittenInitiallyZero)
{
    ASSERT_EQ(NullOStream{}.num_chars_written(), 0);
}

TEST(NullOStream, NumCharsWrittenChangesAfterWriting)
{
    NullOStream s;
    s << "12345";
    s.flush();
    ASSERT_EQ(s.num_chars_written(), 5);
}

TEST(NullOStream, WasWrittenToInitiallyFalse)
{
    ASSERT_FALSE(NullOStream{}.was_written_to());
}

TEST(NullOStream, WasWrittenToBecomesTrueAfterWriting)
{
    NullOStream s;
    s << "12345";
    s.flush();
    ASSERT_TRUE(s.was_written_to());
}

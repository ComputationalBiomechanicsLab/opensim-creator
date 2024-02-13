#include <oscar/Utils/NullOStream.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(NullOStream, CanConstruct)
{
    ASSERT_NO_THROW({ NullOStream{}; });
}

TEST(NullOStream, NumCharsWrittenInitiallyZero)
{
    ASSERT_EQ(NullOStream{}.numCharsWritten(), 0);
}

TEST(NullOStream, NumCharsWrittenChangesAfterWriting)
{
    NullOStream s;
    s << "12345";
    s.flush();
    ASSERT_EQ(s.numCharsWritten(), 5);
}

TEST(NullOStream, WasWrittenToInitiallyFalse)
{
    ASSERT_FALSE(NullOStream{}.wasWrittenTo());
}

TEST(NullOStream, WasWrittenToBecomesTrueAfterWriting)
{
    NullOStream s;
    s << "12345";
    s.flush();
    ASSERT_TRUE(s.wasWrittenTo());
}

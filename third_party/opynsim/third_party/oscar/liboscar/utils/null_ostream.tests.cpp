#include "null_ostream.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(NullOStream, can_be_default_constructed)
{
    ASSERT_NO_THROW({ NullOstream{}; });
}

TEST(NullOStream, num_chars_written_returns_zero_on_new_instance)
{
    ASSERT_EQ(NullOstream{}.num_chars_written(), 0);
}

TEST(NullOStream, num_chars_written_increases_by_writing_to_it)
{
    NullOstream s;
    s << "12345";
    s.flush();
    ASSERT_EQ(s.num_chars_written(), 5);
}

TEST(NullOStream, was_written_returns_false_on_new_instance)
{
    ASSERT_FALSE(NullOstream{}.was_written_to());
}

TEST(NullOStream, was_written_to_returns_true_after_writing_to_it)
{
    NullOstream s;
    s << "12345";
    s.flush();
    ASSERT_TRUE(s.was_written_to());
}

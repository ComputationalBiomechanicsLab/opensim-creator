#include <oscar/Utils/NullStreambuf.h>

#include <gtest/gtest.h>

#include <iosfwd>

using namespace osc;

TEST(NullStreambuf, can_be_default_constructed)
{
    ASSERT_NO_THROW({ NullStreambuf{}; });
}

TEST(NullStreambuf, can_be_wrapped_into_a_std_ostream)
{
    NullStreambuf buf;
    ASSERT_NO_THROW({ std::ostream o{&buf}; });
}

TEST(NullStreambuf, can_be_written_to_via_a_std_ostream)
{
    NullStreambuf buf;
    std::ostream o{&buf};

    ASSERT_NO_THROW({ o << "some content"; });
}

TEST(NullStreambuf, num_chars_written_returns_zero_on_new_instance)
{
    ASSERT_EQ(NullStreambuf{}.num_chars_written(), 0);
}

TEST(NullStreambuf, num_chars_written_increases_after_writing_via_a_std_ostream)
{
    NullStreambuf buf;
    std::ostream{&buf} << "12345";
    ASSERT_EQ(buf.num_chars_written(), 5);
}

TEST(NullStreambuf, was_written_to_returns_false_on_new_instance)
{
    ASSERT_FALSE(NullStreambuf{}.was_written_to());
}

TEST(NullStreambuf, was_written_to_returns_true_after_being_written_to)
{
    NullStreambuf buf;
    std::ostream{&buf} << "12345";
    ASSERT_TRUE(buf.was_written_to());
}

#include <oscar/Utils/SharedPreHashedString.h>

#include <gtest/gtest.h>

#include <string_view>
#include <utility>

using namespace osc;
using namespace std::literals;

TEST(SharedPreHashedString, can_default_construct)
{
    [[maybe_unused]] const SharedPreHashedString should_compile;
}

TEST(SharedPreHashedString, default_constructed_is_empty)
{
    ASSERT_TRUE(SharedPreHashedString{}.empty());
}

TEST(SharedPreHashedString, default_constructed_size_is_zero)
{
    ASSERT_EQ(SharedPreHashedString{}.size(), 0);
}

TEST(SharedPreHashedString, default_constructed_copy_makes_use_count_increment)
{
    const SharedPreHashedString str;
    ASSERT_EQ(str.use_count(), 1);
    const SharedPreHashedString copy = str;
    ASSERT_EQ(str.use_count(), 2);
}

TEST(SharedPreHashedString, can_construct_from_cstring)
{
    const SharedPreHashedString str{"some string"};
    ASSERT_FALSE(str.empty());
    ASSERT_EQ(str, "some string"sv);
}

TEST(SharedPreHashedString, separately_constructed_strings_dont_share_use_count)
{
    const std::string_view source_string = "some string";
    const SharedPreHashedString str1{source_string};
    const SharedPreHashedString str2{source_string};

    // i.e. you should use `StringName`, or your own caching mechanism, if you want
    // automatic deduplication
    ASSERT_EQ(str1.use_count(), 1);
    ASSERT_EQ(str2.use_count(), 1);
}

TEST(SharedPreHashedString, c_str_works_even_when_supplied_non_NUL_terminated_substring)
{
    const std::string_view original_string = "i'm a longer string";
    const std::string_view substring = original_string.substr(0, 5);

    const SharedPreHashedString str{substring};
    ASSERT_EQ(str.c_str()[substring.size()], SharedPreHashedString::value_type{});
}

TEST(SharedPreHashedString, use_count_decrements_when_lifetime_is_dropped)
{
    const SharedPreHashedString str{"another string"};
    ASSERT_EQ(str.use_count(), 1);
    {
        const SharedPreHashedString copy = str;
        ASSERT_EQ(str.use_count(), 2);
    }
    ASSERT_EQ(str.use_count(), 1);
}

TEST(SharedPreHashedString, can_move_construct)
{
    SharedPreHashedString str{"source string"};
    {
        const SharedPreHashedString move_constructed{std::move(str)};
        ASSERT_EQ(move_constructed, std::string_view{"source string"});
    }
}

TEST(SharedPreHashedString, can_copy_assign)
{
    SharedPreHashedString str1{"first"};
    SharedPreHashedString str2{"second"};
    ASSERT_EQ(str2, "second"sv);
    str2 = str1;
    ASSERT_EQ(str2, "first"sv);
    ASSERT_EQ(str2, str1);
}

TEST(SharedPreHashedString, can_move_assign)
{
    SharedPreHashedString str1{"first"};
    SharedPreHashedString str2{"second"};
    ASSERT_EQ(str2, "second"sv);
    str2 = std::move(str1);
    ASSERT_EQ(str2, "first"sv);
}

TEST(SharedPreHashedString, can_implicitly_convert_to_CStringView)
{
    const SharedPreHashedString str{"make me a cstring"};
    const CStringView view = str;
    ASSERT_EQ(view, "make me a cstring"sv);
    ASSERT_EQ(view.c_str()[str.size()], SharedPreHashedString::value_type{});
}

TEST(SharedPreHashedString, can_iterate_over_characters)
{
    std::string_view characters{"abcdef"};
    const SharedPreHashedString str{characters};

    size_t i = 0;
    for (auto character : str) {
        ASSERT_EQ(character, characters[i++]);
    }
}

TEST(SharedPreHashedString, empty_returns_false_for_nonempty_string)
{
    ASSERT_FALSE(SharedPreHashedString{"not empty"}.empty());
}

TEST(SharedPreHashedString, size_returns_expected_answers)
{
    ASSERT_EQ(SharedPreHashedString{}.size(), 0);
    ASSERT_EQ(SharedPreHashedString{" "}.size(), 1);
    ASSERT_EQ(SharedPreHashedString{"a"}.size(), 1);
    ASSERT_EQ(SharedPreHashedString{"ab"}.size(), 2);
    ASSERT_EQ(SharedPreHashedString{"abc"}.size(), 3);
}

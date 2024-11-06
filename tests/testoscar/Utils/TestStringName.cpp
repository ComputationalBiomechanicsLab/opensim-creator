#include <oscar/Utils/StringName.h>

#include <oscar/Utils/CStringView.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace osc;

namespace rgs = std::ranges;

namespace
{
    // SSO: Short String Optimization
    //
    // The reason this code is trying to avoid SSO is to increase the chance that a 3rd-party
    // memory analyzer (e.g. libASAN, valgrind) can spot any issues related to allocating strings
    // in `StringName`'s global string table.
    constexpr auto c_long_character_data_array_to_avoid_SSO = std::to_array("somequitelongstringthatprobablyneedstobeheapallocatedsothatmemoryanalyzershaveabetterchance");
    constexpr const char* const c_long_cstring_to_avoid_SSO = c_long_character_data_array_to_avoid_SSO.data();
    constexpr auto c_another_long_character_data_array_to_avoid_SSO = std::to_array("somedifferencequitelongstringthatprobablyneedstobeheapallocatedbutwhoknows");
    constexpr const char* const c_another_long_cstring_to_avoid_SSO = c_another_long_character_data_array_to_avoid_SSO.data();
}

TEST(StringName, can_default_constructs)
{
    ASSERT_NO_THROW({ StringName{}; });
}

TEST(StringName, copy_constructing_default_constructed_instance_compares_equivalent)
{
    const StringName a;
    const StringName b{a};  // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_EQ(a, b);
}

TEST(StringName, can_move_construct)
{
    StringName a;
    const StringName b{std::move(a)};  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)

    ASSERT_TRUE(b.empty());
    ASSERT_EQ(b.size(), 0);
}

TEST(StringName, copy_assigning_default_constructed_over_non_default_makes_lhs_default)
{
    const StringName a;
    StringName b{c_long_cstring_to_avoid_SSO};
    b = a;
    ASSERT_EQ(a, b);
}

TEST(StringName, move_assigning_default_over_non_default_instance_makes_lhs_default)
{
    StringName a;
    StringName b{c_long_cstring_to_avoid_SSO};
    b = std::move(a);  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    ASSERT_EQ(b, StringName{});
}

TEST(StringName, data_returns_non_nullptr_on_empty_instance)
{
    ASSERT_TRUE(std::string{}.data()) << "this is why StringName::data should return non-nullptr (>=C++11 semantics)";
    ASSERT_TRUE(StringName{}.data());
}

TEST(StringName, c_str_returns_non_nullptr_on_empty_instance)
{
    ASSERT_TRUE(std::string{}.c_str()) << "this is why StringName::data should return non-nullptr (>=C++11 semantics)";
    ASSERT_TRUE(StringName{}.c_str());
}

TEST(StringName, default_constructed_can_convert_to_blank_string_view)
{
    ASSERT_EQ(static_cast<std::string_view>(StringName{}), std::string_view{});
}

TEST(StringName, default_constructed_can_convert_to_blank_CStringView)
{
    ASSERT_EQ(static_cast<CStringView>(StringName{}), CStringView{});
}

TEST(StringName, can_be_used_as_an_argument_to_functions_that_accept_CStringView)
{
    const StringName string_name;
    const auto f = [](CStringView) {};
    f(string_name);  // should compile
}

TEST(StringName, begin_equals_end_on_default_constructed_instance)
{
    const StringName string_name;
    ASSERT_EQ(string_name.begin(), string_name.end());
}

TEST(StringName, cbegin_equals_cend_on_default_constructed_instance)
{
    const StringName string_name;
    ASSERT_EQ(string_name.cbegin(), string_name.cend());
}

TEST(StringName, begin_equals_cbegin_on_default_constructed_instance)
{
    const StringName string_name;
    ASSERT_EQ(string_name.begin(), string_name.cbegin());
}

TEST(StringName, empty_returns_true_on_default_constructed_instance)
{
    ASSERT_TRUE(StringName{}.empty());
}

TEST(StringName, size_returns_0_on_default_constructed_instance)
{
    ASSERT_EQ(StringName{}.size(), 0);
}

TEST(StringName, size_returns_expected_size_when_given_known_string)
{
    ASSERT_EQ(StringName{"pizza"}.size(), 5);
}

TEST(StringName, two_default_constructed_instances_compare_equal)
{
    ASSERT_EQ(StringName{}, StringName{});
}

TEST(StringName, default_constructed_instance_can_be_implicitly_converted_to_a_blank_string_view)
{
    ASSERT_EQ(StringName{}, std::string_view{});
}

TEST(StringName, default_constructed_instance_can_be_implicitly_converted_to_a_blank_CStringView)
{
    ASSERT_EQ(StringName{}, CStringView{});
}

TEST(StringName, default_constructed_instance_compares_equal_to_a_blank_std_string)
{
    ASSERT_EQ(StringName{}, std::string{});
}

TEST(StringName, std_string_compares_equal_to_default_constructed_instance)
{
    ASSERT_EQ(std::string{}, StringName{});
}

TEST(StringName, default_constructed_instance_compares_equal_to_blank_cstring)
{
    ASSERT_EQ(StringName{}, "");
}

TEST(StringName, blank_cstring_compares_equal_to_default_constructed_instance)
{
    ASSERT_EQ("", StringName{});
}

TEST(StringName, default_constructed_instance_compares_not_equal_to_nonempty_instance)
{
    ASSERT_NE(StringName{}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_instance_compares_not_equal_to_default_constructed_instance)
{
    ASSERT_NE(StringName{c_long_cstring_to_avoid_SSO}, StringName{});
}

TEST(StringName, default_constructed_instance_compares_not_equal_to_nonempty_string_view)
{
    ASSERT_NE(StringName{}, std::string_view{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_string_view_compares_not_equal_to_default_constructed_instance)
{
    ASSERT_NE(std::string_view{c_long_cstring_to_avoid_SSO}, StringName{});
}

TEST(StringName, default_constructed_compares_not_equal_to_nonempty_string)
{
    ASSERT_NE(StringName{}, std::string{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_string_compares_not_equal_to_default_constructed_instance)
{
    ASSERT_NE(std::string{c_long_cstring_to_avoid_SSO}, StringName{});
}

TEST(StringName, default_constructed_instance_compares_not_equal_to_nonempty_cstring)
{
    ASSERT_NE(StringName{}, c_long_cstring_to_avoid_SSO);
}

TEST(StringName, nonempty_cstring_compares_not_equal_to_default_constructed_instance)
{
    ASSERT_NE(c_long_cstring_to_avoid_SSO, StringName{});
}

TEST(StringName, default_constructed_instance_compares_less_than_to_nonempty_instance)
{
    ASSERT_LT(StringName{}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_instance_compares_greater_than_or_equal_to_default_constructed_instance)
{
    ASSERT_GE(StringName{c_long_cstring_to_avoid_SSO}, StringName{});
}

TEST(StringName, default_constructed_instance_writes_nothing_to_ostream)
{
    std::stringstream ss;
    ss << StringName{};
    ASSERT_TRUE(ss.str().empty());
}

TEST(StringName, default_constructed_instance_can_be_swapped_with_nonempty_instance)
{
    StringName a;
    const StringName copy_of_a{a};
    StringName b{c_long_cstring_to_avoid_SSO};
    const StringName copy_of_b{b};

    swap(a, b);

    ASSERT_EQ(a, copy_of_b);
    ASSERT_EQ(b, copy_of_a);
}

TEST(StringName, default_constructed_std_hash_is_equal_to_hash_of_std_string)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{}), std::hash<std::string>{}(std::string{}));
}

TEST(StringName, default_constructed_std_hash_is_equal_to_hash_of_string_view)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{}), std::hash<std::string_view>{}(std::string_view{}));
}

TEST(StringName, can_be_constructed_from_a_string_view)
{
    ASSERT_NO_THROW({ StringName(std::string_view(c_long_cstring_to_avoid_SSO)); });
}

TEST(StringName, can_be_constructed_from_a_std_string)
{
    ASSERT_NO_THROW({ StringName(std::string{c_long_cstring_to_avoid_SSO}); });
}

TEST(StringName, can_be_constructed_from_a_cstring)
{
    ASSERT_NO_THROW({ StringName("somecstring"); });
}

TEST(StringName, can_be_implicitly_constructed_from_a_CStringView)
{
    const auto f = [](const CStringView&) {};
    f(CStringView{"cstring"});  // should compile
}

TEST(StringName, copy_assigning_nonempty_over_a_different_nonempty_makes_lhs_compare_equal_to_rhs)
{
    StringName lhs{c_long_cstring_to_avoid_SSO};
    const StringName rhs{c_another_long_cstring_to_avoid_SSO};
    lhs = rhs;
    ASSERT_EQ(lhs, rhs);
}

TEST(StringName, move_assigning_nonempty_instance_over_a_different_nonempty_instance_makes_lhs_compare_equal)
{
    StringName lhs{c_long_cstring_to_avoid_SSO};
    StringName rhs{c_another_long_cstring_to_avoid_SSO};
    const StringName rhs_copy{rhs};
    lhs = std::move(rhs);  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    ASSERT_EQ(lhs, rhs_copy);
}

TEST(StringName, at_returns_character_at_given_index_with_bounds_checking)
{
    const StringName s{"string"};
    ASSERT_EQ(s.at(0), 's');
    ASSERT_EQ(s.at(1), 't');
    ASSERT_EQ(s.at(2), 'r');
    ASSERT_EQ(s.at(3), 'i');
    ASSERT_EQ(s.at(4), 'n');
    ASSERT_EQ(s.at(5), 'g');
    ASSERT_ANY_THROW({ s.at(6); });
    ASSERT_ANY_THROW({ s.at(1000); });
}

TEST(StringName, bracket_operator_returns_character_at_given_index_without_bounds_checking)
{
    const StringName s{"string"};
    ASSERT_EQ(s[0], 's');
    ASSERT_EQ(s[1], 't');
    ASSERT_EQ(s[2], 'r');
    ASSERT_EQ(s[3], 'i');
    ASSERT_EQ(s[4], 'n');
    ASSERT_EQ(s[5], 'g');
}

TEST(StringName, front_returns_first_chracter)
{
    const StringName s{"string"};
    ASSERT_EQ(s.front(), 's');
}

TEST(StringName, back_returns_last_character)
{
    const StringName s{"string"};
    ASSERT_EQ(s.back(), 'g');
}

TEST(StringName, data_returns_NUL_terminated_pointer_to_first_element)
{
    constexpr auto c_input_characters = std::to_array({ 's', 't', 'r', 'i', 'n', 'g'}); // not NUL-terminated (the implementation should handle it)

    const StringName string_name{std::string_view{c_input_characters.data(), 6}};
    const std::span<const StringName::value_type> stringname_span{string_name.data(), std::size(c_input_characters)};

    ASSERT_TRUE(rgs::equal(stringname_span, c_input_characters));
    ASSERT_EQ(string_name.data()[c_input_characters.size()], '\0') << "should be NUL-terminated";
}

TEST(StringName, c_str_returns_NUL_terminated_pointer_to_first_element)
{
    constexpr auto c_input_characters = std::to_array({'s', 't', 'r', 'i', 'n', 'g'});  // not NUL-terminated (the implementation should handle it)

    const StringName string_name{std::string_view{c_input_characters.data(), 6}};
    const std::span<const StringName::value_type> stringname_span{string_name.c_str(), std::size(c_input_characters)};  // plus nul terminator

    ASSERT_TRUE(rgs::equal(stringname_span, c_input_characters));
    ASSERT_EQ(string_name.c_str()[c_input_characters.size()], '\0') << "should be NUL-terminated";
}

TEST(StringName, implicit_conversion_to_string_view_works_as_expected)
{
    const StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(static_cast<std::string_view>(s), std::string_view{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, implicit_conversion_to_CStringView_works_as_expected)
{
    const StringName string_name{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(static_cast<CStringView>(string_name), CStringView{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, begin_compares_not_equal_to_end_when_nonempty)
{
    const StringName string_name{c_long_cstring_to_avoid_SSO};
    ASSERT_NE(string_name.begin(), string_name.end());
}

TEST(StringName, cbegin_compares_not_equal_to_cend_when_nonempty)
{
    const StringName string_name{c_long_cstring_to_avoid_SSO};
    ASSERT_NE(string_name.cbegin(), string_name.cend());
}

TEST(StringName, begin_compares_equal_to_cbegin_when_nonempty)
{
    const StringName string_name{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(string_name.begin(), string_name.cbegin());
}

TEST(StringName, end_compares_equal_to_cend_when_nonempty)
{
    const StringName string_name{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(string_name.end(), string_name.cend());
}

TEST(StringName, empty_returns_false_when_nonempty)
{
    const StringName string_name{c_long_cstring_to_avoid_SSO};
    ASSERT_FALSE(string_name.empty());
}

TEST(StringName, size_returns_expected_value_when_nonempty)
{
    const StringName string_name{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(string_name.size(), c_long_character_data_array_to_avoid_SSO.size()-1);  // minus nul
}

TEST(StringName, nonempty_StringName_compares_equal_to_another_nonempty_StringName_with_the_same_content)
{
    ASSERT_EQ(StringName{c_long_cstring_to_avoid_SSO}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_compares_equal_to_a_string_view_with_the_same_content)
{
    ASSERT_EQ(StringName{c_long_cstring_to_avoid_SSO}, std::string_view{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_string_view_compares_equal_to_a_StringName_with_the_same_content)
{
    ASSERT_EQ(std::string_view{c_long_cstring_to_avoid_SSO}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_StringName_compares_equal_to_cstring_with_the_same_content)
{
    ASSERT_EQ(StringName{c_long_cstring_to_avoid_SSO}, c_long_cstring_to_avoid_SSO);
}

TEST(StringName, nonempty_cstring_compares_equal_to_StringName_with_the_same_content)
{
    ASSERT_EQ(c_long_cstring_to_avoid_SSO, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_StringName_compares_equal_to_CStringView_with_the_same_content)
{
    ASSERT_EQ(StringName{c_long_cstring_to_avoid_SSO}, CStringView{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, nonempty_CStringView_compares_equal_to_StringName_with_the_same_content)
{
    ASSERT_EQ(CStringView{c_long_cstring_to_avoid_SSO}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, compares_not_equal_to_a_StringName_with_different_content)
{
    ASSERT_NE(StringName{c_long_cstring_to_avoid_SSO}, StringName{c_another_long_cstring_to_avoid_SSO});
}

TEST(StringName, compares_not_equal_to_StringName_with_different_content_v2)
{
    ASSERT_NE(StringName{c_another_long_cstring_to_avoid_SSO}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, can_write_content_to_std_ostream)
{
    std::stringstream ss;
    ss << StringName{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(ss.str(), c_long_cstring_to_avoid_SSO);
}

TEST(StringName, std_hash_of_nonempty_StringName_has_same_hash_as_StringName_with_same_content)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_long_cstring_to_avoid_SSO}), std::hash<StringName>{}(StringName{c_long_cstring_to_avoid_SSO}));
}

TEST(StringName, std_hash_of_nonempty_StringName_has_same_hash_as_std_string_with_same_content)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_long_cstring_to_avoid_SSO}), std::hash<std::string>{}(std::string{c_long_cstring_to_avoid_SSO}));
}

TEST(StringName, std_hash_of_nonempty_StringName_has_same_has_as_string_view_with_same_content)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_long_cstring_to_avoid_SSO}), std::hash<std::string_view>{}(std::string_view{c_long_cstring_to_avoid_SSO}));
}

TEST(StringName, writes_identical_output_to_std_ostream_as_a_std_string_with_the_same_content)
{
    const std::string std_string{"some streamed string"};
    std::stringstream string_stream;
    string_stream << std_string;

    std::stringstream stringname_stream;
    stringname_stream << StringName{std_string};

    ASSERT_EQ(string_stream.str(), stringname_stream.str());
}

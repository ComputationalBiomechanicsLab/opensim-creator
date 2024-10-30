#include <oscar/Utils/CStringView.h>

#include <gtest/gtest.h>

#include <array>
#include <string_view>

using namespace osc;
using namespace osc::literals;

TEST(CStringView, when_passed_a_nullptr_cstring_yields_empty_CStringView)
{
    const char* const p = nullptr;
    ASSERT_TRUE(CStringView{p}.empty());
}

TEST(CStringView, when_passed_a_nullptr_cstring_yields_non_nullptr_but_empty_c_str)
{
    const char* const p = nullptr;
    ASSERT_NE(CStringView{p}.c_str(), nullptr);
}

TEST(CStringView, is_empty_when_default_constructed)
{
    ASSERT_TRUE(CStringView{}.empty());
}

TEST(CStringView, returns_non_nullptr_c_str_when_default_constructed)
{
    ASSERT_NE(CStringView{}.c_str(), nullptr);
}

TEST(CStringView, is_empty_when_constructed_from_nullptr)
{
    ASSERT_TRUE(CStringView{nullptr}.empty());
}

TEST(CStringView, c_str_is_not_nullptr_when_CStringView_is_constructed_from_nullptr)
{
    ASSERT_NE(CStringView{nullptr}.c_str(), nullptr);
}

TEST(CStringView, three_way_comparison_operator_behaves_identically_to_std_string_view)
{
    const auto input_strings = std::to_array<const char*>({ "x", "somestring", "somethingelse", "", "_i hope it works ;)" });
    for (const char* const cstring : input_strings) {

        const std::string_view string_view{cstring};
        const CStringView cstring_view{cstring};
        for (const char* input_string : input_strings) {
            ASSERT_EQ(string_view <=> std::string_view{input_string}, cstring_view <=> CStringView{input_string});
        }
    }
}

TEST(CStringView, literal_suffix_returns_CStringView)
{
    static_assert(std::same_as<decltype("hello"_cs), CStringView>);
    ASSERT_EQ("hello"_cs, CStringView{"hello"});
}

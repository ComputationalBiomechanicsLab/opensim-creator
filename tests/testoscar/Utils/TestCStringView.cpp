#include "oscar/Utils/CStringView.hpp"

#include <gtest/gtest.h>

TEST(CStringView, WhenPassedNullCstringYieldsEmptyCStringView)
{
    char const* p = nullptr;
    ASSERT_TRUE(osc::CStringView{p}.empty());
}

TEST(CStringView, WhenPassedNullCStringYieldsNonNull_c_str)
{
    char const* p = nullptr;
    ASSERT_NE(osc::CStringView{p}.c_str(), nullptr);
}

TEST(CStringView, WhenDefaultConstructedYieldsEmptyCStringView)
{
    ASSERT_TRUE(osc::CStringView{}.empty());
}

TEST(CStringView, WhenDefaultConstructedYieldsNonNull_c_str)
{
    ASSERT_NE(osc::CStringView{}.c_str(), nullptr);
}

TEST(CStringView, WhenConstructedFromNullptrYieldsEmptyCStringView)
{
    ASSERT_TRUE(osc::CStringView{nullptr}.empty());
}

TEST(CStringView, WhenConstructedFromNullptrYieldsNonNull_c_str)
{
    ASSERT_NE(osc::CStringView{nullptr}.c_str(), nullptr);
}

#include "oscar/Utils/CStringView.hpp"

#include <gtest/gtest.h>

TEST(CStringView, WhenPassedNullCstringYieldsEmptyCStringView)
{
    char const* p = nullptr;
    ASSERT_TRUE(osc::CStringView{p}.empty());
}

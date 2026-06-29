#include "symbol.h"

#include <gtest/gtest.h>

#include <sstream>

using namespace opyn;

TEST(Symbol, writes_expected_string_to_ostream)
{
    std::stringstream ss;
    ss << Symbol{"foo"};

    ASSERT_EQ(std::move(ss).str(), "Symbol(\"foo\")");
}

TEST(Symbol, formats_as_expected)
{
    ASSERT_EQ(std::format("{}", Symbol{"bar"}), "Symbol(\"bar\")");
}

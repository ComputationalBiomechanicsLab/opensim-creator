#include "src/Utils/DedupedString.hpp"

#include <gtest/gtest.h>

TEST(DedupedString, CanConstructWithBlankString)
{
	osc::DedupedString s{""};
}

TEST(DedupedString, BlankStringsAreEquivalent)
{
	osc::DedupedString s1{""};
	osc::DedupedString s2{""};

	ASSERT_EQ(s1, s2);
}

TEST(DedupedString, CanConstructWithNonblankString)
{
	osc::DedupedString s{"a"};
}

TEST(DedupedString, NonblankStringsAreEquivalent)
{
	osc::DedupedString s1{"a"};
	osc::DedupedString s2{"a"};

	ASSERT_EQ(s1, s2);
}

TEST(DedupedString, BlankStringIsNotEqualToNonblankString)
{
	osc::DedupedString s1{""};
	osc::DedupedString s2{"a"};

	ASSERT_NE(s1, s2);
}

// and so on... (WIP)
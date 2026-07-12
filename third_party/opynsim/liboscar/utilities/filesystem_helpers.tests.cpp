#include "filesystem_helpers.h"

#include <gtest/gtest.h>

#include <filesystem>

using namespace osc;

TEST(is_subpath, returns_true_if_given_a_subpath)
{
    ASSERT_TRUE (is_subpath("/some/dir/somewhere", "/some/dir/somewhere/deeper/file.txt"));
    ASSERT_TRUE (is_subpath("/some/dir/somewhere", "/some/dir/somewhere"));
    ASSERT_FALSE(is_subpath("/some/dir/somewhere", "/some/dir/file.txt"));
    ASSERT_FALSE(is_subpath("/some/dir/somewhere/deeper/file.txt", "/some/dir/somewhere"));
}

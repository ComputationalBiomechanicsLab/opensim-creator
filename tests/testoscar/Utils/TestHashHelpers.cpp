#include <oscar/Utils/HashHelpers.h>

#include <gtest/gtest.h>

#include <utility>

using namespace osc;

TEST(Hasher, can_hash_a_std_pair)
{
    const std::pair<int, int> p = {-20, 8};
    const Hasher<std::pair<int, int>> hasher;
    ASSERT_NE(hasher(p), 0);
}

TEST(Hasher, hash_of_std_pair_changes_when_first_element_changes)
{
    const std::pair<int, int> p1 = {-20, 8};
    std::pair<int, int> p2 = p1;
    p2.first += 10;
    const Hasher<std::pair<int, int>> hasher;
    ASSERT_NE(hasher(p1), hasher(p2));
}

TEST(Hasher, hash_of_std_pair_changes_when_second_element_changes)
{
    const std::pair<int, int> p1 = {-20, 8};
    std::pair<int, int> p2 = p1;
    p2.second += 7;
    const Hasher<std::pair<int, int>> hasher;
    ASSERT_NE(hasher(p1), hasher(p2));
}

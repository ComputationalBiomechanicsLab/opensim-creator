#include <oscar/Utils/HashHelpers.h>

#include <gtest/gtest.h>

#include <utility>

using namespace osc;

TEST(Hasher, CanBeUsedToHashAStdPair)
{
    const std::pair<int, int> p = {-20, 8};
    const Hasher<std::pair<int, int>> hasher;
    ASSERT_NE(hasher(p), 0);
}

TEST(Hasher, StdPairHashChangesWhenFirstElementDiffers)
{
    const std::pair<int, int> p1 = {-20, 8};
    std::pair<int, int> p2 = p1;
    p2.first += 10;
    const Hasher<std::pair<int, int>> hasher;
    ASSERT_NE(hasher(p1), hasher(p2));
}

TEST(Hasher, StdPairHashChangesWhenSecondElementDiffers)
{
    const std::pair<int, int> p1 = {-20, 8};
    std::pair<int, int> p2 = p1;
    p2.second += 7;
    const Hasher<std::pair<int, int>> hasher;
    ASSERT_NE(hasher(p1), hasher(p2));
}

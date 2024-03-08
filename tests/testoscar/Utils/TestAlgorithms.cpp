#include <oscar/Utils/Algorithms.h>

#include <gtest/gtest.h>

#include <array>
#include <map>
#include <unordered_map>

using namespace osc;

TEST(all_of, WorksAsExpected)
{
    auto vs = std::to_array({-1, -2, -3, 0, 1, 2, 3});
    ASSERT_TRUE(all_of(vs, [](int v){ return v > -4; }));
    ASSERT_TRUE(all_of(vs, [](int v){ return v <  4; }));
    ASSERT_FALSE(all_of(vs, [](int v){ return v > 0; }));
}
TEST(at, WorksAsExpected)
{
    auto vs = std::to_array({-1, -2, -3, 0, 1, 2, 3});
    ASSERT_EQ(at(vs, 0), -1);
    ASSERT_EQ(at(vs, 1), -2);
    ASSERT_EQ(at(vs, 2), -3);
    ASSERT_EQ(at(vs, 3), -0);
    ASSERT_EQ(at(vs, 4),  1);
    ASSERT_EQ(at(vs, 5),  2);
    ASSERT_EQ(at(vs, 6),  3);
    ASSERT_THROW({ at(vs, 7); }, std::out_of_range);
    ASSERT_THROW({ at(vs, 8); }, std::out_of_range);

}

TEST(at, WorksAtCompileTime)
{
    static_assert(at(std::to_array({-1, 0, 1}), 1) == 0);
}

TEST(find_or_optional, WorksWithStdUnorderedMap)
{
    std::unordered_map<int, int> um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(find_or_optional(um, -20), std::nullopt);
    ASSERT_EQ(find_or_optional(um, -15), 20);
    ASSERT_EQ(find_or_optional(um, -2), std::nullopt);
    ASSERT_EQ(find_or_optional(um, -1), 98);
    ASSERT_EQ(find_or_optional(um, 0), std::nullopt);
    ASSERT_EQ(find_or_optional(um, 5), 10);
}

TEST(find_or_optional, WorksWithStdMap)
{
    std::map<int, int> um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(find_or_optional(um, -20), std::nullopt);
    ASSERT_EQ(find_or_optional(um, -15), 20);
    ASSERT_EQ(find_or_optional(um, -2), std::nullopt);
    ASSERT_EQ(find_or_optional(um, -1), 98);
    ASSERT_EQ(find_or_optional(um, 0), std::nullopt);
    ASSERT_EQ(find_or_optional(um, 5), 10);
}

TEST(try_find, WorksWithUnorderedMap)
{
    std::unordered_map<int, int> um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(try_find(um, -20), nullptr);
    ASSERT_EQ(*try_find(um, -15), 20);
    ASSERT_EQ(try_find(um, -2), nullptr);
    ASSERT_EQ(*try_find(um, -1), 98);
    ASSERT_EQ(try_find(um, 0), nullptr);
    ASSERT_EQ(*try_find(um, 5), 10);
}

TEST(try_find, WorksWithStdMap)
{
    std::map<int, int> um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(try_find(um, -20), nullptr);
    ASSERT_EQ(*try_find(um, -15), 20);
    ASSERT_EQ(try_find(um, -2), nullptr);
    ASSERT_EQ(*try_find(um, -1), 98);
    ASSERT_EQ(try_find(um, 0), nullptr);
    ASSERT_EQ(*try_find(um, 5), 10);
}

TEST(try_find, WorksWithConstQualifiedStdUnorderedMap)
{
    std::unordered_map<int, int> const um{
        {20, 30},
        {-1, 98},
        {5, 10},
        {-15, 20},
    };

    ASSERT_EQ(try_find(um, -20), nullptr);
    ASSERT_EQ(*try_find(um, -15), 20);
    ASSERT_EQ(try_find(um, -2), nullptr);
    ASSERT_EQ(*try_find(um, -1), 98);
    ASSERT_EQ(try_find(um, 0), nullptr);
    ASSERT_EQ(*try_find(um, 5), 10);
}

TEST(try_find, CanMutateViaReturnedPointer)
{
    std::unordered_map<int, int> um{
        {20, 30},
    };

    int* v = try_find(um, 20);
    *v = -40;
    ASSERT_TRUE(try_find(um, 20));
    ASSERT_EQ(*try_find(um, 20), -40);
}

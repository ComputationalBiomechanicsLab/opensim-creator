#include "CopyOnUpdSharedValue.h"

#include <gtest/gtest.h>

#include <compare>
#include <type_traits>

using namespace osc;

namespace
{
    struct TestStruct {
        explicit TestStruct(int v) : value(v) {}

        int value;
    };
}

TEST(CopyOnUpdSharedValue, can_construct)
{
    auto cow = make_cowv<TestStruct>(42);
    ASSERT_EQ(cow->value, 42);
    ASSERT_EQ((*cow).value, 42);
}

TEST(CopyOnUpdSharedValue, can_copy_construct)
{
    auto cow1 = make_cowv<TestStruct>(100);
    CopyOnUpdSharedValue<TestStruct> cow2(cow1);

    // Both should point to the same data
    ASSERT_EQ(cow1.get(), cow2.get());
    ASSERT_TRUE(cow1 == cow2);
}

TEST(CopyOnUpdSharedValue, copy_assignment_works_as_expected)
{
    auto cow1 = make_cowv<TestStruct>(10);
    auto cow2 = make_cowv<TestStruct>(20);

    cow2 = cow1;
    ASSERT_EQ(cow1.get(), cow2.get());
    ASSERT_EQ(cow2->value, 10);
}

TEST(CopyOnUpdSharedValue, upd_correctly_triggers_a_copy)
{
    auto cow1 = make_cowv<TestStruct>(5);
    auto cow2{cow1};  // shared

    TestStruct* upd_ptr = cow2.upd();  // should trigger copy

    ASSERT_NE(cow1, cow2) << "cow2 should be independent";
    ASSERT_EQ(upd_ptr->value, 5);

    upd_ptr->value = 42;
    ASSERT_EQ(cow2->value, 42);
    ASSERT_EQ(cow1->value, 5) << "original should be unchanged";
}

TEST(CopyOnUpdSharedValue, swap_works)
{
    auto cow1 = make_cowv<TestStruct>(1);
    auto cow2 = make_cowv<TestStruct>(2);

    static_assert(std::is_nothrow_swappable_v<decltype(cow1)>);
    swap(cow1, cow2);

    ASSERT_EQ(cow1->value, 2);
    ASSERT_EQ(cow2->value, 1);
}

TEST(CopyOnUpdSharedValue, IdentityComparison)
{
    auto cow1 = make_cowv<TestStruct>(7);
    auto cow2{cow1};
    auto cow3 = make_cowv<TestStruct>(7);

    ASSERT_EQ(cow1, cow2);
    ASSERT_NE(cow1, cow3);
    ASSERT_EQ((cow1 <=> cow2), std::strong_ordering::equal);
}

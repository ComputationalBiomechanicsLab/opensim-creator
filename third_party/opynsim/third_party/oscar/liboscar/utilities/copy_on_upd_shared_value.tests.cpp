#include "copy_on_upd_shared_value.h"

#include <gtest/gtest.h>

#include <array>
#include <compare>
#include <type_traits>

using namespace osc;

namespace
{
    struct TestStruct {
        explicit TestStruct(int v) : value(v) {}

        int value;
    };

    struct alignas(64) OveralignedStruct {
        explicit OveralignedStruct(int v) : value(v) {}

        int value;
        std::array<char, 60> padding{};  // Add some padding to make sure alignment matters
    };

    static_assert(alignof(OveralignedStruct) == 64, "OveralignedStruct should have 64-byte alignment");
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
    CopyOnUpdSharedValue<TestStruct> cow2(cow1);  // NOLINT(performance-unnecessary-copy-initialization)

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
    auto cow2{cow1};  // NOLINT(performance-unnecessary-copy-initialization)
    auto cow3 = make_cowv<TestStruct>(7);

    ASSERT_EQ(cow1, cow2);
    ASSERT_NE(cow1, cow3);
    ASSERT_EQ((cow1 <=> cow2), std::strong_ordering::equal);
}

TEST(CopyOnUpdSharedValue, can_construct_overaligned)
{
    auto cow = make_cowv<OveralignedStruct>(123);
    ASSERT_EQ(cow->value, 123);
    ASSERT_EQ((*cow).value, 123);

    // Check the alignment of the underlying pointer
    ASSERT_EQ(reinterpret_cast<uintptr_t>(cow.get()) % alignof(OveralignedStruct), 0u) << "Underlying storage must respect overalignment";
}

TEST(CopyOnUpdSharedValue, can_copy_construct_overaligned)
{
    auto cow1 = make_cowv<OveralignedStruct>(88);
    CopyOnUpdSharedValue<OveralignedStruct> cow2(cow1);  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(cow1.get(), cow2.get());
    ASSERT_TRUE(cow1 == cow2);
}

TEST(CopyOnUpdSharedValue, copy_assignment_works_as_expected_with_overaligned)
{
    auto cow1 = make_cowv<OveralignedStruct>(10);
    auto cow2 = make_cowv<OveralignedStruct>(20);

    cow2 = cow1;
    ASSERT_EQ(cow1.get(), cow2.get());
    ASSERT_EQ(cow2->value, 10);

    ASSERT_EQ(reinterpret_cast<uintptr_t>(cow2.get()) % alignof(OveralignedStruct), 0u);
}

TEST(CopyOnUpdSharedValue, upd_correctly_triggers_a_copy_and_copy_is_overaligned)
{
    auto cow1 = make_cowv<OveralignedStruct>(5);
    auto cow2{cow1};  // shared

    OveralignedStruct* upd_ptr = cow2.upd();  // should trigger copy

    ASSERT_NE(cow1, cow2);
    ASSERT_EQ(upd_ptr->value, 5);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(upd_ptr) % alignof(OveralignedStruct), 0u) << "upd copy should also be overaligned";

    upd_ptr->value = 42;
    ASSERT_EQ(cow2->value, 42);
    ASSERT_EQ(cow1->value, 5);
}

TEST(CopyOnUpdSharedValue, swap_works_with_overaligned_data)
{
    auto cow1 = make_cowv<OveralignedStruct>(1);
    auto cow2 = make_cowv<OveralignedStruct>(2);

    static_assert(std::is_nothrow_swappable_v<decltype(cow1)>);
    swap(cow1, cow2);

    ASSERT_EQ(cow1->value, 2);
    ASSERT_EQ(cow2->value, 1);

    ASSERT_EQ(reinterpret_cast<uintptr_t>(cow1.get()) % alignof(OveralignedStruct), 0u);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(cow2.get()) % alignof(OveralignedStruct), 0u);
}

TEST(CopyOnUpdSharedValue, equality_compares_pointer_equivalence)
{
    auto cow1 = make_cowv<OveralignedStruct>(7);
    auto cow2{cow1};  // NOLINT(performance-unnecessary-copy-initialization)
    auto cow3 = make_cowv<OveralignedStruct>(7);

    ASSERT_EQ(cow1, cow2);
    ASSERT_NE(cow1, cow3);
    ASSERT_EQ((cow1 <=> cow2), std::strong_ordering::equal);

    ASSERT_EQ(reinterpret_cast<uintptr_t>(cow1.get()) % alignof(OveralignedStruct), 0u);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(cow3.get()) % alignof(OveralignedStruct), 0u);
}

TEST(CopyOnUpdSharedValue, use_count_behaves_as_expected)
{
    const auto cow1 = make_cowv<TestStruct>(1);
    ASSERT_EQ(cow1.use_count(), 1);
    {
        const auto cow2{cow1};  // NOLINT(performance-unnecessary-copy-initialization)
        ASSERT_EQ(cow1.use_count(), 2);
        ASSERT_EQ(cow2.use_count(), 2);
    }
    ASSERT_EQ(cow1.use_count(), 1);
    const auto cow2{cow1};  // NOLINT(performance-unnecessary-copy-initialization)
    {
        const auto cow3{cow2};  // NOLINT(performance-unnecessary-copy-initialization)
        ASSERT_EQ(cow1.use_count(), 3);
        ASSERT_EQ(cow2.use_count(), 3);
        ASSERT_EQ(cow3.use_count(), 3);
    }
    ASSERT_EQ(cow1.use_count(), 2);
    ASSERT_EQ(cow2.use_count(), 2);
}

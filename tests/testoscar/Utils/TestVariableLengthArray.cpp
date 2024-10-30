#include <oscar/Utils/VariableLengthArray.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <memory_resource>

using namespace osc;

TEST(VariableLengthArray, can_default_construct)
{
    const VariableLengthArray<int, 1024> vla;
}

TEST(VariableLengthArray, can_construct_with_1_on_stack_element)
{
    const VariableLengthArray<int, 1> vla;
}

TEST(VariableLengthArray, can_construct_with_0_on_stack_elements)
{
    VariableLengthArray<int, 0> vla;
    vla.push_back(5);
    ASSERT_EQ(vla.front(), 5);
}

TEST(VariableLengthArray, can_construct_from_initializer_list)
{
    const VariableLengthArray<int, 4> vla = {0, 1, 2, 3};
    ASSERT_EQ(vla.size(), 4);
    ASSERT_EQ(vla[0], 0);
    ASSERT_EQ(vla[1], 1);
    ASSERT_EQ(vla[2], 2);
    ASSERT_EQ(vla[3], 3);
}

TEST(VariableLengthArray, immediately_spills_to_upstream_if_given_oversized_initializer_list)
{
    const auto fn = []() { VariableLengthArray<int, 1> vla({0, 1}, std::pmr::null_memory_resource()); };
    ASSERT_THROW({ 	fn(); }, std::bad_alloc);
}

TEST(VariableLengthArray, doesnt_spill_if_push_back_used_after_undersized_initializer_list)
{
    VariableLengthArray<int, 3> vla({0, 1}, std::pmr::null_memory_resource());
    vla.push_back(2);  // still under the limit
    ASSERT_THROW({ vla.push_back(3); }, std::bad_alloc);
}

TEST(VariableLengthArray, can_copy_construct_when_it_contains_copyable_objects)
{
    static_assert(std::copy_constructible<VariableLengthArray<int, 3>>);
    static_assert(not std::copy_constructible<VariableLengthArray<std::unique_ptr<int>, 3>>);
}

TEST(VariableLengthArray, when_copied_uses_the_same_upstream_memory_resource_as_original)
{
    VariableLengthArray<int, 3> vla({0, 1}, std::pmr::null_memory_resource());
    auto copy{vla};
    ASSERT_EQ(copy.size(), 2);
    copy.push_back(2);
    ASSERT_EQ(vla.size(), 2);
    ASSERT_EQ(copy.size(), 3);
    ASSERT_THROW({ copy.push_back(3); }, std::bad_alloc) << "should throw, because it took the null resource from the source vla";
}

TEST(VariableLengthArray, can_move_construct_when_it_contains_moveonly_objects)
{
    static_assert(not std::copy_constructible<VariableLengthArray<std::unique_ptr<int>, 2>>);
    static_assert(std::move_constructible<VariableLengthArray<std::unique_ptr<int>, 2>>);
}

TEST(VariableLengthArray, when_move_constructed_uses_same_upstream_memory_resource_as_original)
{
    VariableLengthArray<std::unique_ptr<int>, 3> vla(std::pmr::null_memory_resource());
    vla.push_back(std::make_unique<int>(0));
    vla.push_back(std::make_unique<int>(1));

    auto moved{std::move(vla)};
    ASSERT_EQ(moved.size(), 2);
    moved.push_back(std::make_unique<int>(2));
    ASSERT_EQ(moved.size(), 3);
    ASSERT_THROW({ moved.push_back(std::make_unique<int>(3)); }, std::bad_alloc) << "should throw, because it took the null resource from the source vla";
}

TEST(VariableLengthArray, can_copy_assign)
{
    static_assert(std::is_copy_assignable_v<VariableLengthArray<int, 3>>);
    static_assert(not std::is_copy_assignable_v<VariableLengthArray<std::unique_ptr<int>, 3>>);
}

TEST(VariableLengthArray, copy_assignment_does_not_propagate_allocator)
{
    static_assert(not std::allocator_traits<std::pmr::polymorphic_allocator<int>>::propagate_on_container_copy_assignment());

    VariableLengthArray<int, 2> rhs({0}, std::pmr::null_memory_resource());
    VariableLengthArray<int, 2> lhs = {0, 1};

    lhs = rhs;
    ASSERT_EQ(lhs, rhs);
    ASSERT_EQ(lhs.size(), 1);
    lhs.push_back(1);
    ASSERT_NO_THROW({ lhs.push_back(2); }) << "shouldn't propagate the null memory resource";
}

TEST(VariableLengthArray, can_move_assign)
{
    static_assert(std::is_move_assignable_v<VariableLengthArray<std::unique_ptr<int>, 3>>);
}

TEST(VariableLengthArray, move_assignment_does_not_propagate_allocator)
{
    static_assert(not std::allocator_traits<std::pmr::polymorphic_allocator<int>>::propagate_on_container_move_assignment());

    VariableLengthArray<std::unique_ptr<int>, 2> rhs(std::pmr::null_memory_resource());
    rhs.push_back(std::make_unique<int>(0));
    VariableLengthArray<std::unique_ptr<int>, 2> lhs;
    lhs.push_back(std::make_unique<int>(0));
    lhs.push_back(std::make_unique<int>(1));

    lhs = std::move(rhs);
    ASSERT_EQ(lhs.size(), 1);
    ASSERT_EQ(*lhs.front(), 0);
    lhs.push_back(std::make_unique<int>(1));
    ASSERT_NO_THROW({ lhs.push_back(std::make_unique<int>(2)); }) << "shouldn't propagate the null memory resource";
}

TEST(VariableLengthArray, push_back_increases_size_by_1)
{
    VariableLengthArray<int, 8> vla;
    ASSERT_EQ(vla.size(), 0);
    vla.push_back(1);
    ASSERT_EQ(vla.size(), 1);
    vla.push_back(2);
    ASSERT_EQ(vla.size(), 2);
    vla.push_back(3);
    ASSERT_EQ(vla.size(), 3);
}

TEST(VariableLengthArray, push_back_works_on_moveonly_types)
{
    VariableLengthArray<std::unique_ptr<int>, 8> vla;
    vla.push_back(nullptr);
}

TEST(VariableLengthArray, push_back_works_with_overaligned_values)
{
    struct alignas(256) Overaligned final {
        int64_t value = 0;
        std::array<std::byte, 256 - sizeof(int64_t)> padding{};
    };

    VariableLengthArray<Overaligned, 4> vla(std::pmr::null_memory_resource());  // throws `std::bad_alloc` on allocation
    vla.push_back(Overaligned{0});
    vla.push_back(Overaligned{1});
    vla.push_back(Overaligned{2});
    vla.push_back(Overaligned{3});

    ASSERT_EQ(vla[0].value, 0);
    ASSERT_EQ(vla[1].value, 1);
    ASSERT_EQ(vla[2].value, 2);
    ASSERT_EQ(vla[3].value, 3);
}

TEST(VariableLengthArray, push_back_uses_upstream_allocator_only_once_N_is_exceeded)
{
    VariableLengthArray<int, 4> vla(std::pmr::null_memory_resource());  // throws `std::bad_alloc` on allocation

    vla.push_back(0);
    vla.push_back(1);
    vla.push_back(2);
    vla.push_back(3);
    ASSERT_THROW({ vla.push_back(4); }, std::bad_alloc);
}

TEST(VariableLengthArray, clear_clears_content)
{
    VariableLengthArray<int, 1> vla = {0};
    ASSERT_EQ(vla.size(), 1);
    vla.clear();
    ASSERT_EQ(vla.size(), 0);
    ASSERT_TRUE(vla.empty());
}

TEST(VariableLengthArray, equality_works_as_expected)
{
    const VariableLengthArray<int, 1> a = {1};
    const VariableLengthArray<int, 1> b = {1};
    const VariableLengthArray<int, 1> c = {2};

    ASSERT_EQ(a, a);
    ASSERT_EQ(a, b);
    ASSERT_EQ(b, a);
    ASSERT_EQ(b, b);
    ASSERT_EQ(c, c);

    ASSERT_NE(a, c);
    ASSERT_NE(c, a);
    ASSERT_NE(b, c);
    ASSERT_NE(c, b);
}

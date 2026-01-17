#include "vector.h"

#include <gtest/gtest.h>

#include <concepts>
#include <ranges>
#include <type_traits>

using namespace osc;

static_assert(std::same_as<float, Vector3f::value_type>);
static_assert(std::same_as<double, Vector3d::value_type>);
static_assert(std::same_as<int, Vector3i::value_type>);
static_assert(std::same_as<float, Vector4f::value_type>);
static_assert(std::same_as<double, Vector4d::value_type>);
static_assert(std::same_as<int, Vector4i::value_type>);
static_assert(std::ranges::contiguous_range<Vector3f>);

TEST(Vector, with_element_works_as_expected)
{
    ASSERT_EQ(Vector2{}.with_element(0, 2.0f), Vector2(2.0f, 0.0f));
    ASSERT_EQ(Vector2(1.0f).with_element(0, 3.0f), Vector2(3.0f, 1.0f));
    ASSERT_EQ(Vector2{}.with_element(1, 3.0f), Vector2(0.0f, 3.0f));
}

TEST(Vector, can_be_used_to_construct_a_span_of_floats)
{
    static_assert(std::ranges::contiguous_range<Vector2> and std::ranges::sized_range<Vector2>);
    static_assert(std::constructible_from<std::span<const float>, const Vector2&>);
    static_assert(not std::constructible_from<std::span<float>, const Vector2&>);
    static_assert(std::constructible_from<std::span<float>, Vector2&>);
}

TEST(Vector, can_be_used_as_arg_to_sized_span_func)
{
    const auto f = []([[maybe_unused]] std::span<float, 2>) {};
    static_assert(std::invocable<decltype(f), Vector2>);
}

TEST(Vector, default_constructor_zero_initializes)
{
    const Vector3f v;
    ASSERT_EQ(v[0], 0.0f);
    ASSERT_EQ(v[1], 0.0f);
    ASSERT_EQ(v[2], 0.0f);
}

TEST(Vector, single_argument_constructor_fills_the_Vector)
{
    const Vector3i v{7};
    ASSERT_EQ(v[0], 7);
    ASSERT_EQ(v[1], 7);
    ASSERT_EQ(v[2], 7);
}

TEST(Vector, element_by_element_constructor_fills_each_element_of_the_Vector)
{
    const Vector<int, 5> v{0, 2, 4, 6, 8};
    ASSERT_EQ(v[0], 0);
    ASSERT_EQ(v[1], 2);
    ASSERT_EQ(v[2], 4);
    ASSERT_EQ(v[3], 6);
    ASSERT_EQ(v[4], 8);
}

/*
TEST_CASE("Vector element-by-element constructor can convert args")
{
    struct B final {};
    struct A final { explicit operator B() const { return B{}; }};

    [[maybe_unused]] const Vector<B, 3> v{A{}, A{}, A{}};
}

TEST_CASE("std::tuple_size works with Vector")
{
    static_assert(std::tuple_size_v<Vector3d> == 3);
    static_assert(std::tuple_size_v<Vector<std::string, 7>> == 7);
    static_assert(std::tuple_size_v<Vector<std::string_view, 2>> == 2);
}

TEST_CASE("ADL get works with Vector")
{
    const Vector3d v{1.0, 2.0, 3.0};
    CHECK(get<0>(v) == v[0]);
    CHECK(get<1>(v) == v[1]);
    CHECK(get<2>(v) == v[2]);
}

TEST_CASE("can be accessed via structured bindings")
{
    Vector3d v{1.0, 2.0, 3.0};
    auto& [x, y, z] = v;
    CHECK(x == v[0]);
    CHECK(y == v[1]);
    CHECK(z == v[2]);
    x = -1.0;
    y = -2.0;
    z = -3.0;
    CHECK(v[0] == -1.0);
    CHECK(v[1] == -2.0);
    CHECK(v[2] == -3.0);
}

TEST_CASE("Vector can be constructed from a Vector of a different type, if a conversion exists")
{
    const Vector<std::string, 2> strings;
    [[maybe_unused]] const Vector<std::string_view, 2> views{strings};
}

TEST_CASE("Vector can be constructed with move-only objects in some other Vector")
{
    Vector<std::unique_ptr<int>, 2> pointers;
    [[maybe_unused]] const Vector<std::unique_ptr<int>, 2> move_target{std::move(pointers)};
}

TEST_CASE("Vector can be constructed with move-only convertible pointers")
{
    // "Not because it's easy (or, to be fair, necessary), but because it's hard"

    struct A {};
    struct B : A {};
    Vector<std::unique_ptr<B>, 2> pointers;
    [[maybe_unused]] const Vector<std::unique_ptr<B>, 2> converted_move_only_target{std::move(pointers)};
}

TEST_CASE("Vector can be constructed from a std::array")
{
    Vector<int, 3> from_array{std::array<int, 3>{5, 6, 7}};
    CHECK(from_array[0] == 5);
    CHECK(from_array[1] == 6);
    CHECK(from_array[2] == 7);
}

TEST_CASE("Vector can be constructed from a fixed-size std::span")
{
    std::vector<float> values = {11.0f, 12.0f, 14.0f, 18.0f};
    Vector<float, 4> from_span{std::span<float, 4>{values}};
    CHECK(from_span[0] == 11.0f);
    CHECK(from_span[1] == 12.0f);
    CHECK(from_span[2] == 14.0f);
    CHECK(from_span[3] == 18.0f);
}

TEST_CASE("Vector constructed from bigger range results in truncation")
{
    // Should be allowed, but should truncate
    Vector<int, 3> from_larger_array{std::array<int, 5>{5, 6, 7, 11, -1000}};
    CHECK(from_larger_array[0] == 5);
    CHECK(from_larger_array[1] == 6);
    CHECK(from_larger_array[2] == 7);
}

TEST_CASE("Vector assignment correctly handles trunaction, move assignment, and convertability")
{
    struct A { A(int v) : v_{v} {} int v_; };
    struct B : A { using A::A; };
    Vector<std::unique_ptr<B>, 3> bs = {
        std::make_unique<B>(5),
        std::make_unique<B>(6),
        std::make_unique<B>(7),
    };
    const Vector<std::unique_ptr<const B>, 2> as{std::move(bs)};
    CHECK(as[0]->v_ == 5);
    CHECK(as[1]->v_ == 6);
}

TEST_CASE("Equality comparison works as expected")
{
    CHECK(Vector2f{5.0f, 4.0f} == Vector2f{5.0f, 4.0f});
    CHECK(Vector2i{5, 6} != Vector2i{-5, 6});
    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    CHECK(Vector2f{nan, 5.0f} != Vector2f{nan, 5.0f});
}

TEST_CASE("Vector can be implicitly converted into a statically-sized span")
{
    Vector4i v{-1, -5, -7, 15};

    const std::span<int, 4> mutable_view{v};
    CHECK(mutable_view[0] == -1);
    CHECK(mutable_view[1] == -5);
    CHECK(mutable_view[2] == -7);
    CHECK(mutable_view[3] == 15);

    const std::span<const int, 4> view{v};
    CHECK(view[0] == -1);
    CHECK(view[1] == -5);
    CHECK(view[2] == -7);
    CHECK(view[3] == 15);
}

TEST_CASE("size can return the size of the Vector at compile-time")
{
    static_assert(Vector4f{}.size() == 4);
}

TEST_CASE("Vector binary operator+ with scalar works as expected")
{
    static_assert(std::same_as<decltype(Vector3f{1.0f, 2.0f, 3.0f} + 2.0 ), Vector3d>);
    static_assert(std::same_as<decltype(Vector3i{1,    2,    3}    + 2.0f), Vector3f>);
    static_assert(std::same_as<decltype(Vector3i{1,    2,    3}    + 2.0 ), Vector3d>);

    CHECK((Vector3i{1, 2, 3} + 2.0f) == Vector3f{1+2.0f, 2+2.0f, 3+2.0f});
}

TEST_CASE("Vector binary operator+ with Vector works as expected")
{
    static_assert(std::same_as<decltype(Vector3f{1.0f, 2.0f, 3.0f} + Vector3d{2.0} ), Vector3d>);
    static_assert(std::same_as<decltype(Vector3i{1,    2,    3}    + Vector3f{2.0f}), Vector3f>);
    static_assert(std::same_as<decltype(Vector3i{1,    2,    3}    + Vector3d{2.0} ), Vector3d>);

    CHECK((Vector3i{1, 2, 3} + Vector3f{4.0f, 5.0f, 9.0f}) == Vector3f{1+4.0f, 2+5.0f, 3+9.0f});
}

TEST_CASE("Vector can be formatted to an output stream")
{
    std::stringstream ss;
    ss << Vector3i{-5, 0, 9};
    CHECK(ss.str() == "Vector3(-5, 0, 9)");
}
*/

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

TEST(Vector, element_by_element_constructor_can_convert_args)
{
    struct B final {};
    struct A final { explicit operator B() const { return B{}; }};

    [[maybe_unused]] const Vector<B, 3> v{A{}, A{}, A{}};
}

TEST(Vector, is_compabile_with_std_tuple_size)
{
    static_assert(std::tuple_size_v<Vector3d> == 3);
    static_assert(std::tuple_size_v<Vector<std::string, 7>> == 7);
    static_assert(std::tuple_size_v<Vector<std::string_view, 2>> == 2);
}

TEST(Vector, works_with_std_get)
{
    const Vector3d v{1.0, 2.0, 3.0};
    ASSERT_EQ(get<0>(v), v[0]);
    ASSERT_EQ(get<1>(v), v[1]);
    ASSERT_EQ(get<2>(v), v[2]);
}

TEST(Vector, can_be_accessed_via_structured_bindings)
{
    Vector3d v{1.0, 2.0, 3.0};
    auto& [x, y, z] = v;
    ASSERT_EQ(x, v[0]);
    ASSERT_EQ(y, v[1]);
    ASSERT_EQ(z, v[2]);
    x = -1.0;
    y = -2.0;
    z = -3.0;
    ASSERT_EQ(v[0], -1.0);
    ASSERT_EQ(v[1], -2.0);
    ASSERT_EQ(v[2], -3.0);
}

TEST(Vector, can_be_constructed_from_vector_of_different_type_if_conversion_exists)
{
    const Vector<std::string, 2> strings;
    [[maybe_unused]] const Vector<std::string_view, 2> views{strings};
}

TEST(Vector, can_be_constructed_with_move_only_object_from_some_other_Vector)
{
    Vector<std::unique_ptr<int>, 2> pointers;
    [[maybe_unused]] const Vector<std::unique_ptr<int>, 2> move_target{std::move(pointers)};
}

TEST(Vector, can_be_constructed_with_move_only_convertible_pointers)
{
    // "Not because it's easy (or, to be fair, necessary), but because it's hard"

    struct A {};
    struct B : A {};
    Vector<std::unique_ptr<B>, 2> pointers;
    [[maybe_unused]] const Vector<std::unique_ptr<B>, 2> converted_move_only_target{std::move(pointers)};
}

TEST(Vector, can_be_constructed_from_a_std_array)
{
    Vector<int, 3> from_array{std::array<int, 3>{5, 6, 7}};
    ASSERT_EQ(from_array[0], 5);
    ASSERT_EQ(from_array[1], 6);
    ASSERT_EQ(from_array[2], 7);
}

TEST(Vector, can_be_constructed_from_fixed_size_span)
{
    std::vector<float> values = {11.0f, 12.0f, 14.0f, 18.0f};
    Vector<float, 4> from_span{std::span<float, 4>{values}};
    ASSERT_EQ(from_span[0], 11.0f);
    ASSERT_EQ(from_span[1], 12.0f);
    ASSERT_EQ(from_span[2], 14.0f);
    ASSERT_EQ(from_span[3], 18.0f);
}

TEST(Vector, constructed_from_bigger_range_results_in_truncation)
{
    // Should be allowed, but should truncate
    Vector<int, 3> from_larger_array{std::array<int, 5>{5, 6, 7, 11, -1000}};
    ASSERT_EQ(from_larger_array[0], 5);
    ASSERT_EQ(from_larger_array[1], 6);
    ASSERT_EQ(from_larger_array[2], 7);
}

TEST(Vector, assignment_correctly_handles_truncation_move_assignment_and_convertability)
{
    struct A { A(int v) : v_{v} {} int v_; };
    struct B : A { using A::A; };
    Vector<std::unique_ptr<B>, 3> bs = {
        std::make_unique<B>(5),
        std::make_unique<B>(6),
        std::make_unique<B>(7),
    };
    const Vector<std::unique_ptr<const B>, 2> as{std::move(bs)};
    ASSERT_EQ(as[0]->v_, 5);
    ASSERT_EQ(as[1]->v_, 6);
}

TEST(Vector, equality_works_as_expected)
{
    ASSERT_EQ(Vector2f(5.0f, 4.0f), Vector2f(5.0f, 4.0f));
    ASSERT_NE(Vector2i(5, 6), Vector2i(-5, 6));
    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    ASSERT_NE(Vector2f(nan, 5.0f), Vector2f(nan, 5.0f));
}

TEST(Vector, can_be_implicitly_converted_to_a_statically_sized_span)
{
    Vector4i v{-1, -5, -7, 15};

    const std::span<int, 4> mutable_view{v};
    ASSERT_EQ(mutable_view[0], -1);
    ASSERT_EQ(mutable_view[1], -5);
    ASSERT_EQ(mutable_view[2], -7);
    ASSERT_EQ(mutable_view[3], 15);

    const std::span<const int, 4> view{v};
    ASSERT_EQ(view[0], -1);
    ASSERT_EQ(view[1], -5);
    ASSERT_EQ(view[2], -7);
    ASSERT_EQ(view[3], 15);
}

TEST(Vector, size_can_return_vector_size_at_compile_time)
{
    static_assert(Vector4f{}.size() == 4);
}

TEST(Vector, binary_operator_plus_with_scalar_works_as_expected)
{
    static_assert(std::same_as<decltype(Vector3f{1.0f, 2.0f, 3.0f} + 2.0 ), Vector3d>);
    static_assert(std::same_as<decltype(Vector3i{1,    2,    3}    + 2.0f), Vector3f>);
    static_assert(std::same_as<decltype(Vector3i{1,    2,    3}    + 2.0 ), Vector3d>);

    ASSERT_EQ((Vector3i{1, 2, 3} + 2.0f), Vector3f(1+2.0f, 2+2.0f, 3+2.0f));
}

TEST(Vector, binary_operator_plus_works_with_Vector_as_expected)
{
    static_assert(std::same_as<decltype(Vector3f{1.0f, 2.0f, 3.0f} + Vector3d{2.0} ), Vector3d>);
    static_assert(std::same_as<decltype(Vector3i{1,    2,    3}    + Vector3f{2.0f}), Vector3f>);
    static_assert(std::same_as<decltype(Vector3i{1,    2,    3}    + Vector3d{2.0} ), Vector3d>);

    ASSERT_EQ((Vector3i{1, 2, 3} + Vector3f{4.0f, 5.0f, 9.0f}), Vector3f(1+4.0f, 2+5.0f, 3+9.0f));
}

TEST(Vector, can_be_written_to_a_std_ostream)
{
    std::stringstream ss;
    ss << Vector3i{-5, 0, 9};
    ASSERT_EQ(ss.str(), "Vector3(-5, 0, 9)");
}

TEST(Vector, x_returns_first_element_as_const_reference)
{
    const Vector3d v{1.0, 2.0, 3.0};
    const double& x = v.x();
    ASSERT_EQ(x, 1.0);
}

TEST(Vector, x_is_constexpr)
{
    static_assert(Vector3d{1.0, 2.0, 3.0}.x() == 1.0);
}

TEST(Vector, x_can_return_mutable_reference)
{
    Vector3d v{1.0, 2.0, 3.0};
    v.x() += 3.0;
    ASSERT_EQ(v.x(), 1.0+3.0);
}

TEST(Vector, y_returns_second_element_as_const_reference)
{
    const Vector3d v{1.0, 2.0, 3.0};
    const double& y = v.y();
    ASSERT_EQ(y, 2.0);
}

TEST(Vector, y_is_constexpr)
{
    static_assert(Vector3d{1.0, 2.0, 3.0}.y() == 2.0);
}

TEST(Vector, y_can_return_mutable_reference)
{
    Vector3d v{1.0, 2.0, 3.0};
    v.y() += 3.0;
    ASSERT_EQ(v.y(), 2.0+3.0);
}

TEST(Vector, z_returns_third_element_as_const_reference)
{
    const Vector3d v{1.0, 2.0, 3.0};
    const double& z = v.z();
    ASSERT_EQ(z, 3.0);
}

TEST(Vector, z_is_constexpr)
{
    static_assert(Vector3d{1.0, 2.0, 3.0}.z() == 3.0);
}

TEST(Vector, z_can_return_mutable_reference)
{
    Vector3d v{1.0, 2.0, 3.0};
    v.z() += 3.0;
    ASSERT_EQ(v.z(), 3.0+3.0);
}

TEST(Vector, w_returns_fourth_element_as_const_reference)
{
    const Vector4d v{1.0, 2.0, 3.0, 4.0};
    const double& w = v.w();
    ASSERT_EQ(w, 4.0);
}

TEST(Vector, w_is_constexpr)
{
    static_assert(Vector4d{1.0, 2.0, 3.0, 4.0}.w() == 4.0);
}

TEST(Vector, w_can_return_mutable_reference)
{
    Vector4d v{1.0, 2.0, 3.0, 4.0};
    v.w() += 3.0;
    ASSERT_EQ(v.w(), 4.0+3.0);
}

TEST(Vector, xy_returns_expected_elements)
{
    Vector3i v{5, 6, 7};
    ASSERT_EQ(v.xy(), Vector2i(5, 6));
}

TEST(Vector, xy_is_constexpr)
{
    static_assert(Vector3i{5, 6, 7}.xy() == Vector2i{5, 6});
}

TEST(Vector, xz_returns_expected_elements)
{
    Vector3i v{5, 6, 7};
    ASSERT_EQ(v.xz(), Vector2i(5, 7));
}

TEST(Vector, xz_is_constexpr)
{
    static_assert(Vector3i{5, 6, 7}.xz() == Vector2i{5, 7});
}

TEST(Vector, yx_returns_expected_elements)
{
    Vector3i v{5, 6, 7};
    ASSERT_EQ(v.yx(), Vector2i(6, 5));
}

TEST(Vector, yx_is_constexpr)
{
    static_assert(Vector3i{5, 6, 7}.yx() == Vector2i{6, 5});
}

TEST(Vector, yz_returns_expected_elements)
{
    Vector3i v{5, 6, 7};
    ASSERT_EQ(v.yz(), Vector2i(6, 7));
}

TEST(Vector, yz_is_constexpr)
{
    static_assert(Vector3i{5, 6, 7}.yz() == Vector2i{6, 7});
}

TEST(Vector, zx_returns_expected_elements)
{
    Vector3i v{5, 6, 7};
    ASSERT_EQ(v.zx(), Vector2i(7, 5));
}

TEST(Vector, zx_is_constexpr)
{
    static_assert(Vector3i{5, 6, 7}.zx() == Vector2i{7, 5});
}

TEST(Vector, zy_returns_expected_elements)
{
    Vector3i v{5, 6, 7};
    ASSERT_EQ(v.zy(), Vector2i(7, 6));
}

TEST(Vector, zy_is_constexpr)
{
    static_assert(Vector3i{5, 6, 7}.zy() == Vector2i{7, 6});
}

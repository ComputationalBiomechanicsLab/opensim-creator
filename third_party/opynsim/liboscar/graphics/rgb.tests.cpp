#include "rgb.h"

#include <liboscar/graphics/unorm8.h>
#include <liboscar/maths/vector.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

TEST(Rgb, default_constructs_to_black)
{
    static_assert(Rgb<float>{} == Rgb<float>::black());
}

TEST(Rgb, constructed_with_3_values_assigns_each_to_r_g_b_respectively)
{
    constexpr Rgb<float> rgb{0.2f, 0.3f, 0.4f};
    ASSERT_EQ(rgb.r, 0.2f);
    ASSERT_EQ(rgb.g, 0.3f);
    ASSERT_EQ(rgb.b, 0.4f);
}

TEST(Rgb, constructed_with_1_value_fills_value_into_all_components)
{
    static_assert(Rgb<float>{1.5f} == Rgb<float>{1.5f, 1.5f, 1.5f});
}

TEST(Rgb, constructed_from_Vector3_extracts_xyz_into_rgb)
{
    static_assert(Rgb<float>{Vector3f{0.5f, 0.6f, 0.7f}} == Rgb<float>{0.5f, 0.6f, 0.7f});
}

TEST(Rgb, converting_constructor_behaves_as_expected)
{
    const Rgb<float> rgb{Rgb<Unorm8>{0xff, 0x00, 0xff}};
    ASSERT_EQ(rgb.r, static_cast<float>(Unorm8{0xff}));
    ASSERT_EQ(rgb.g, static_cast<float>(Unorm8{0x00}));
    ASSERT_EQ(rgb.b, static_cast<float>(Unorm8{0xff}));
}

TEST(Rgb, converting_constructor_from_Vector3_behaves_as_expected)
{
    const Rgb<Unorm8> rgb{Vector3f{0.05f, 0.1f, 0.15f}};
    ASSERT_EQ(rgb.r, static_cast<Unorm8>(0.05f));
    ASSERT_EQ(rgb.g, static_cast<Unorm8>(0.1f));
    ASSERT_EQ(rgb.b, static_cast<Unorm8>(0.15f));
}

TEST(Rgb, satisfies_contiguous_range_api)
{
    static_assert(rgs::contiguous_range<Rgb<float>>);
    static_assert(rgs::contiguous_range<Rgb<Unorm8>>);
}

TEST(Rgb, operator_bracket_const_works_as_expected)
{
    const Rgb<float> rgb{0.9f, 0.95f, 0.99f};
    ASSERT_EQ(rgb[0], 0.9f);
    ASSERT_EQ(rgb[1], 0.95f);
    ASSERT_EQ(rgb[2], 0.99f);
}

TEST(Rgb, operator_bracket_works_as_expected)
{
    Rgb<float> rgb{0.9f, 0.95f, 0.99f};
    ASSERT_EQ(rgb[0], 0.9f);
    ASSERT_EQ(rgb[1], 0.95f);
    ASSERT_EQ(rgb[2], 0.99f);
}

TEST(Rgb, const_iterators_work_as_expected_with_algorithms)
{
    const auto values = std::to_array({0.1f, 0.2f, 0.3f});
    const auto rgb    =     Rgb<float>{0.1f, 0.2f, 0.3f};

    ASSERT_TRUE(rgs::equal(values, rgb));
}

TEST(Rgb, mutable_iterators_work_as_expected)
{
    Rgb<float> rgb{0.1f, 0.2f, 0.3f};
    for (auto& component : rgb) {
        component *= 2.0f;
    }
    ASSERT_EQ(rgb, Rgb<float>(2.0f*0.1f, 2.0f*0.2f, 2.0f*0.3f));
}

TEST(Rgb, size_is_3)
{
    static_assert(Rgb<float>{}.size() == 3);
}

TEST(Rgb, tuple_size_is_3)
{
    static_assert(std::tuple_size_v<Rgb<float>> == 3);
}

TEST(Rgb, structured_bindings_work_as_expected)
{
    const auto [r, g, b] = Rgb<float>{2.0f, 2.5f, 3.0f};
    ASSERT_EQ(r, 2.0f);
    ASSERT_EQ(g, 2.5f);
    ASSERT_EQ(b, 3.0f);
}

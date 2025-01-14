#include "Rgba.h"

#include <liboscar/Graphics/Unorm8.h>
#include <liboscar/Maths/Vec4.h>

#include <gtest/gtest.h>

#include <functional>
#include <sstream>

using namespace osc;

TEST(Rgba, can_be_instantiated_with_float_template_arg)
{
    [[maybe_unused]] const Rgba<float> this_should_compile;
}

TEST(Rgba, can_be_instantiated_with_Unorm8_template_arg)
{
    [[maybe_unused]] const Rgba<Unorm8> this_should_compile;
}

TEST(Rgba, can_write_float_components_to_std_ostream)
{
    std::stringstream ss;
    ss << Rgba<float>{};
    ASSERT_FALSE(ss.str().empty());
}

TEST(Rgba, can_write_Unorm8_componrnts_to_std_ostream)
{
    std::stringstream ss;
    ss << Rgba<Unorm8>{};
    ASSERT_FALSE(ss.str().empty());
}

TEST(Rgba, can_be_decomposed_into_component_parts)
{
    const Rgba<float> rgba{1.0f, 2.0f, 3.0f, 0.5f};
    const auto& [r, g, b, a] = rgba;

    ASSERT_EQ(r, 1.0f);
    ASSERT_EQ(g, 2.0f);
    ASSERT_EQ(b, 3.0f);
    ASSERT_EQ(a, 0.5f);
}

TEST(Rgba, can_be_decomposed_into_mutable_component_parts)
{
    Rgba<float> rgba{1.0f, 2.0f, 3.0f, 0.5f};
    auto& [r, g, b, a] = rgba;

    r *= 2.0f;
    g *= 2.0f;
    b *= 2.0f;
    a *= 2.0f;

    ASSERT_EQ(rgba.r, 2.0f);
    ASSERT_EQ(rgba.g, 4.0f);
    ASSERT_EQ(rgba.b, 6.0f);
    ASSERT_EQ(rgba.a, 1.0f);
}

TEST(Rgba, value_ptr_works_for_float_type)
{
    const Rgba<float> rgba;
    ASSERT_EQ(value_ptr(rgba), &rgba.r);
}

TEST(Rgba, value_ptr_works_for_Unorm8)
{
    const Rgba<Unorm8> rgba;
    ASSERT_EQ(value_ptr(rgba), &rgba.r);
}

TEST(Rgba, float_rgba_can_be_hashed)
{
    Rgba<float> rgba{0.125f, 0.25f, 0.5f, 1.0f};
    const std::hash<Rgba<float>> hasher{};

    size_t last_hash = 0;
    for (auto& component : rgba) {
        component *= 0.5f;
        const size_t hash = hasher(rgba);
        ASSERT_NE(hash, last_hash);
        last_hash = hash;
    }
}

TEST(Rgba, Unorm8_rgba_can_be_hashed)
{
    Rgba<Unorm8> rgba{0.125f, 0.25f, 0.5f, 1.0f};
    const std::hash<Rgba<Unorm8>> hasher{};

    size_t last_hash = 0;
    for (auto& component : rgba) {
        component = component.normalized_value() * 0.5f;
        const size_t hash = hasher(rgba);
        ASSERT_NE(hash, last_hash);
        last_hash = hash;
    }
}

TEST(Rgba, can_implicitly_construct_Rgba_from_different_components_if_components_are_implicitly_convertible)
{
    const Rgba<float> float_val{0.0f, 0.5f, 1.0f, 1.0f};
    const Rgba<Unorm8> unorm8_val = float_val;

    ASSERT_EQ(unorm8_val.r, 0.0f);
    ASSERT_EQ(unorm8_val.g, Unorm8{127});
    ASSERT_EQ(unorm8_val.b, 1.0f);
    ASSERT_EQ(unorm8_val.a, Unorm8{0xff});
}

TEST(Rgba, can_explicitly_construct_from_Vec4_of_different_type)
{
    const Rgba<Unorm8> unorm8_val{Vec4{0.0f, 0.5f, 1.0f, 1.0f}};

    ASSERT_EQ(unorm8_val.r, 0.0f);
    ASSERT_EQ(unorm8_val.g, Unorm8{127});
    ASSERT_EQ(unorm8_val.b, 1.0f);
    ASSERT_EQ(unorm8_val.a, Unorm8{0xff});
}

TEST(Rgba, map_with_unary_operation_works_as_expected)
{
    const Rgba<float> inputs = {1.0f, 2.0f, 3.0f, 4.0f};
    const auto f = [](float input) { return input + 1.0f; };
    const Rgba<float> result = map(inputs, f);
    const Rgba<float> expected = {1.0f+1.0f, 2.0f+1.0f, 3.0f+1.0f, 4.0f+1.0f};

    ASSERT_EQ(result, expected);
}

TEST(Rgba, map_with_binary_operation_works_as_expected)
{
    const Rgba<float> lhs = {1.0f, 2.0f, 3.0f, 4.0f};
    const Rgba<float> rhs = {1.0f, 2.0f, 3.0f, 4.0f};
    const Rgba<float> result = map(lhs, rhs, std::plus<float>{});
    const Rgba<float> expected = {1.0f+1.0f, 2.0f+2.0f, 3.0f+3.0f, 4.0f+4.0f};

    ASSERT_EQ(result, expected);
}

TEST(Rgba, lerp_works_with_unorm8)
{
    const Rgba<Unorm8> lhs = {0x00, 0x00, 0x00, 0x00};
    const Rgba<Unorm8> rhs = {0xff, 0xff, 0xff, 0xff};
    const Rgba<Unorm8> result = lerp(lhs, rhs, 123.0f/255.0f);
    const Rgba<Unorm8> expected = {123, 123, 123, 123};

    ASSERT_EQ(result, expected);
}

TEST(Rgba, saturate_works_as_expected)
{
    const Rgba<float> hdr_color = {1.5f, 1.1f, -0.1f, 0.5f};
    const Rgba<float> result = saturate(hdr_color);
    const Rgba<float> expected = {1.0f, 1.0f, 0.0f, 0.5f};

    ASSERT_EQ(result, expected);
}

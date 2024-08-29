#include <oscar/Graphics/Color.h>

#include <gtest/gtest.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <algorithm>
#include <array>
#include <type_traits>
#include <utility>

using namespace osc;

namespace
{
    // these testing values were pulled out of inkscape, which is assumed to
    // have correct RGB-to-HSL relations
    struct KnownRGBAToHSLAConversions final {
        Color input;
        ColorHSLA expected_output;
    };
    constexpr auto c_RGBA_to_HSLA_known_conversion_values = std::to_array<KnownRGBAToHSLAConversions>({
         // RGBA                     // HSLA
         // r     g     b     a      // h (degrees) s     l     a
        {{  1.0f, 0.0f, 0.0f, 1.0f}, {  0.0f,       1.0f, 0.5f, 1.0f}},  // red
        {{  0.0f, 1.0f, 0.0f, 1.0f}, {  120.0f,     1.0f, 0.5f, 1.0f}},  // green
        {{  0.0f, 0.0f, 1.0f, 1.0f}, {  240.0f,     1.0f, 0.5f, 1.0f}},  // blue
    });

    std::ostream& operator<<(std::ostream& o, const KnownRGBAToHSLAConversions& test_case)
    {
        return o << "rgba = " << test_case.input << ", hsla = " << test_case.expected_output;
    }

    constexpr float c_HLSL_conversion_tolerance_per_component = 0.0001f;
}

TEST(Color, default_constructs_to_clear_color)
{
    static_assert(Color{} == Color::clear());
}

TEST(Color, constructed_from_one_value_fills_RGB_components_with_that_value_and_alpha_one)
{
    static_assert(Color{0.23f} == Color{0.23f, 0.23f, 0.23f, 1.0f});
}

TEST(Color, constructed_from_two_values_fills_RGB_components_with_first_and_alpha_with_second)
{
    static_assert(Color{0.83f, 0.4f} == Color{0.83f, 0.83f, 0.83f, 0.4f});
}

TEST(Color, constructed_with_Vec3_and_float_fills_RGB_components_with_Vec3_and_alpha_with_float)
{
    static_assert(Color{Vec3{0.1f, 0.2f, 0.3f}, 0.7f} == Color{0.1f, 0.2f, 0.3f, 0.7f});
}
TEST(Color, can_construct_from_RGBA_floats)
{
    const Color color{5.0f, 4.0f, 3.0f, 2.0f};
    ASSERT_EQ(color.r, 5.0f);
    ASSERT_EQ(color.g, 4.0f);
    ASSERT_EQ(color.b, 3.0f);
    ASSERT_EQ(color.a, 2.0f);
}

TEST(Color, RGBA_float_constructor_is_constexpr)
{
    // must compile
    [[maybe_unused]] constexpr Color color{0.0f, 0.0f, 0.0f, 0.0f};
}

TEST(Color, can_construct_from_RGB_floats)
{
    const Color color{5.0f, 4.0f, 3.0f};
    ASSERT_EQ(color.r, 5.0f);
    ASSERT_EQ(color.g, 4.0f);
    ASSERT_EQ(color.b, 3.0f);

    ASSERT_EQ(color.a, 1.0f);  // default value when given RGB
}

TEST(Color, RGB_float_constructor_is_constexpr)
{
    // must compile
    [[maybe_unused]] constexpr Color color{0.0f, 0.0f, 0.0f};
}

TEST(Color, can_explicitly_construct_from_Vec3)
{
    const Vec3 v = {0.25f, 0.387f, 0.1f};
    const Color color{v};

    // ensure vec3 ctor creates a solid hdr_color with a == 1.0f
    ASSERT_EQ(color.r, v.x);
    ASSERT_EQ(color.g, v.y);
    ASSERT_EQ(color.b, v.z);
    ASSERT_EQ(color.a, 1.0f);
}

TEST(Color, can_explicitly_construct_from_Vec4)
{
    [[maybe_unused]] const Color color{Vec4{0.0f, 1.0f, 0.0f, 1.0f}};
}

TEST(Color, implicitly_converts_into_a_Vec4)
{
    [[maybe_unused]] constexpr Vec4 v = Color{0.0f, 0.0f, 1.0f, 0.0f};
}

TEST(Color, bracket_operator_accesses_each_component)
{
    const Color color = {0.32f, 0.41f, 0.78f, 0.93f};

    ASSERT_EQ(color[0], color.r);
    ASSERT_EQ(color[1], color.g);
    ASSERT_EQ(color[2], color.b);
    ASSERT_EQ(color[3], color.a);
}

TEST(Color, Vec4_constructor_is_constexpr)
{
    // must compile
    [[maybe_unused]] constexpr Color color{Vec4{0.0f, 1.0f, 0.0f, 1.0f}};
}

TEST(Color, operator_equals_returns_true_for_equivalent_colors)
{
    const Color a = {1.0f, 0.0f, 1.0f, 0.5f};
    const Color b = {1.0f, 0.0f, 1.0f, 0.5f};

    ASSERT_TRUE(a == b);
}

TEST(Color, operator_equals_returns_false_for_inequivalent_colors)
{
    const Color a = {0.0f, 0.0f, 1.0f, 0.5f};
    const Color b = {1.0f, 0.0f, 1.0f, 0.5f};

    ASSERT_FALSE(a == b);
}

TEST(Color, operator_not_equals_returns_true_for_inequivalent_colors)
{
    const Color a = {0.0f, 0.0f, 1.0f, 0.5f};
    const Color b = {1.0f, 0.0f, 1.0f, 0.5f};

    ASSERT_TRUE(a != b);
}

TEST(Color, operator_not_equals_returns_false_for_equivalent_colors)
{
    const Color a = {0.0f, 0.0f, 1.0f, 0.5f};
    const Color b = {0.0f, 0.0f, 1.0f, 0.5f};

    ASSERT_FALSE(a != b);
}

TEST(Color, const_begin_and_end_iterators_behave_as_expected)
{
    const Color c = {1.0f, 0.25f, 0.1f, 0.3f};
    const auto expected = std::to_array({c.r, c.g, c.b, c.a});

    ASSERT_TRUE(std::equal(c.begin(), c.end(), expected.begin(), expected.end()));
}

TEST(Color, non_const_begin_and_end_iterators_behave_as_expected)
{
    const Color c = {1.0f, 0.25f, 0.1f, 0.3f};
    const auto expected = std::to_array({c.r, c.g, c.b, c.a});

    ASSERT_TRUE(std::equal(c.begin(), c.end(), expected.begin(), expected.end()));
}

TEST(Color, operator_multiply_between_two_Colors_performs_componentwise_multiplication)
{
    const Color a = {0.64f, 0.90f, 0.21f, 0.89f};
    const Color b = {0.12f, 0.10f, 0.23f, 0.01f};

    const Color rv = a * b;

    ASSERT_EQ(rv.r, a.r * b.r);
    ASSERT_EQ(rv.g, a.g * b.g);
    ASSERT_EQ(rv.b, a.b * b.b);
    ASSERT_EQ(rv.a, a.a * b.a);
}

TEST(Color, operator_multiply_equals_performs_in_place_componentwise_multiplication)
{
    const Color a = {0.64f, 0.90f, 0.21f, 0.89f};
    const Color b = {0.12f, 0.10f, 0.23f, 0.01f};

    Color rv = a;
    rv *= b;

    ASSERT_EQ(rv.r, a.r * b.r);
    ASSERT_EQ(rv.g, a.g * b.g);
    ASSERT_EQ(rv.b, a.b * b.b);
    ASSERT_EQ(rv.a, a.a * b.a);
}

TEST(Color, to_linear_colorspace_on_float_returns_linearazied_version_of_one_srgb_color_component)
{
    const float srgb_color = 0.02f;
    const float linear_color = to_linear_colorspace(srgb_color);

    // we don't test what the actual equation is, just that low
    // sRGB colors map to higher linear colors (i.e. they are
    // "stretched out" from the bottom of the curve)
    ASSERT_GT(srgb_color, linear_color);
}

TEST(Color, to_srgb_colorspace_on_float_returns_srgb_version_of_one_linear_color_component)
{
    const float linear_color = 0.4f;
    const float srgb_color = to_srgb_colorspace(linear_color);

    // we don't test what the actual equation is, just that low-ish
    // linear colors are less than the equivalent sRGB hdr_color (because
    // sRGB will stretch lower colors out)
    ASSERT_LT(linear_color, srgb_color);
}

TEST(Color, to_linear_colorspace_on_Color_returns_linearized_version_of_the_Color)
{
    const Color srgb_color = {0.5f, 0.5f, 0.5f, 0.5f};
    const Color linear_color = to_linear_colorspace(srgb_color);

    ASSERT_EQ(linear_color.r, to_linear_colorspace(srgb_color.r));
    ASSERT_EQ(linear_color.g, to_linear_colorspace(srgb_color.g));
    ASSERT_EQ(linear_color.b, to_linear_colorspace(srgb_color.b));
    ASSERT_EQ(linear_color.a, srgb_color.a) << "alpha should remain untouched by this operation (alpha is always linear)";
}

TEST(Color, to_srgb_colorspace_returns_srgb_version_of_the_linear_Color)
{
    const Color linear_color = {0.25f, 0.25f, 0.25f, 0.6f};
    const Color srgb_color = to_srgb_colorspace(linear_color);

    ASSERT_EQ(srgb_color.r, to_srgb_colorspace(linear_color.r));
    ASSERT_EQ(srgb_color.g, to_srgb_colorspace(linear_color.g));
    ASSERT_EQ(srgb_color.b, to_srgb_colorspace(linear_color.b));
    ASSERT_EQ(srgb_color.a, linear_color.a) << "alpha should remain untouched by this operation (alpha is always linear)";
}

TEST(Color, to_linear_colorspace_followed_by_to_srgb_colorspace_on_Color_returns_original_input_Color)
{
    const Color original_color = {0.1f, 0.1f, 0.1f, 0.5f};
    const Color converted_color = to_srgb_colorspace(to_linear_colorspace(original_color));

    constexpr float tolerance = 0.0001f;
    ASSERT_NEAR(original_color.r, converted_color.r, tolerance);
    ASSERT_NEAR(original_color.g, converted_color.g, tolerance);
    ASSERT_NEAR(original_color.b, converted_color.b, tolerance);
    ASSERT_NEAR(original_color.a, converted_color.a, tolerance);
}

TEST(Color, to_color32_returns_RGBA32_version_of_the_color)
{
    const Color color = {0.85f, 0.62f, 0.3f, 0.5f};
    const Color32 expected = {
        static_cast<uint8_t>(color.r * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.g * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.b * static_cast<float>(0xff)),
        static_cast<uint8_t>(color.a * static_cast<float>(0xff)),
    };

    const Color32 got = to_color32(color);

    ASSERT_EQ(expected.r, got.r);
    ASSERT_EQ(expected.g, got.g);
    ASSERT_EQ(expected.b, got.b);
    ASSERT_EQ(expected.a, got.a);
}

TEST(Color, to_color32_clamps_HDR_color_components)
{
    const Color hdr_color = {1.5f, 0.0f, 2.0f, 1.0f};
    const Color32 expected = {0xff, 0x00, 0xff, 0xff};
    ASSERT_EQ(to_color32(hdr_color), expected);
}

TEST(Color, to_color32_clamps_negative_color_components)
{
    const Color color = {-1.0f, 0.0f, 1.0f, 1.0f};
    const Color32 expected = {0x00, 0x00, 0xff, 0xff};
    ASSERT_EQ(to_color32(color), expected);
}

TEST(Color, to_color_on_Color32_returns_expected_colors)
{
    ASSERT_EQ(to_color(Color32(0xff, 0x00, 0x00, 0xff)), Color(1.0f, 0.0f, 0.0f, 1.0f));
    ASSERT_EQ(to_color(Color32(0x00, 0xff, 0x00, 0xff)), Color(0.0f, 1.0f, 0.0f, 1.0f));
    ASSERT_EQ(to_color(Color32(0x00, 0x00, 0xff, 0xff)), Color(0.0f, 0.0f, 1.0f, 1.0f));
    ASSERT_EQ(to_color(Color32(0x00, 0xff, 0xff, 0x00)), Color(0.0f, 1.0f, 1.0f, 0.0f));
}

TEST(Color, black_returns_black_color)
{
    ASSERT_EQ(Color::black(), Color(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST(Color, blue_returns_blue_color)
{
    ASSERT_EQ(Color::blue(), Color(0.0f, 0.0f, 1.0f, 1.0f));
}

TEST(Color, clear_returns_clear_color)
{
    ASSERT_EQ(Color::clear(), Color(0.0f, 0.0f, 0.0f, 0.0f));
}

TEST(Color, green_returns_green_color)
{
    ASSERT_EQ(Color::green(), Color(0.0f, 1.0f, 0.0f, 1.0f));
}

TEST(Color, red_returns_red_color)
{
    ASSERT_EQ(Color::red(), Color(1.0f, 0.0f, 0.0f, 1.0f));
}

TEST(Color, white_returns_white_color)
{
    ASSERT_EQ(Color::white(), Color(1.0f, 1.0f, 1.0f, 1.0f));
}

TEST(Color, yellow_returns_yellow_color)
{
    ASSERT_EQ(Color::yellow(), Color(1.0f, 1.0f, 0.0f, 1.0f));
}

TEST(Color, with_alpha_returns_original_Color_with_new_alpha)
{
    static_assert(Color::white().with_alpha(0.33f) == Color{1.0f, 1.0f, 1.0f, 0.33f});
}

TEST(Color, value_ptr_returns_memory_address_of_first_component)
{
    const Color color = Color::red();
    ASSERT_EQ(&color.r, value_ptr(color));
}

TEST(Color, mutable_value_ptr_returns_memory_address_of_first_component)
{
    const Color color = Color::red();
    ASSERT_EQ(&color.r, value_ptr(color));
}

TEST(Color, lerp_with_zero_interpolant_returns_first_color)
{
    const Color a = Color::red();
    const Color b = Color::blue();

    ASSERT_EQ(lerp(a, b, 0.0f), a);
}

TEST(Color, lerp_with_1_interpolant_returns_second_color)
{
    const Color a = Color::red();
    const Color b = Color::blue();

    ASSERT_EQ(lerp(a, b, 1.0f), b);
}

TEST(Color, lerp_with_negative_interpolant_returns_first_color)
{
    // tests that `t` is appropriately clamped

    const Color a = Color::red();
    const Color b = Color::blue();

    ASSERT_EQ(lerp(a, b, -1.0f), a);
}

TEST(Color, lerp_with_above_one_interpolant_returns_second_color)
{
    // tests that `t` is appropriately clamped

    const Color a = Color::red();
    const Color b = Color::blue();

    ASSERT_EQ(lerp(a, b, 2.0f), b);
}

TEST(Color, lerp_with_intermediate_interpolant_returns_expected_result)
{
    const Color a = Color::red();
    const Color b = Color::blue();
    const float t = 0.5f;
    const float tolerance = 0.0001f;

    const Color rv = lerp(a, b, t);

    for (size_t i = 0; i < 4; ++i) {
        ASSERT_NEAR(rv[i], (1.0f-t)*a[i] + t*b[i], tolerance);
    }
}

TEST(Color, works_with_std_hash)
{
    const Color a = Color::red();
    const Color b = Color::blue();

    ASSERT_NE(std::hash<Color>{}(a), std::hash<Color>{}(b));
}

TEST(Color, to_html_string_returns_equivalent_LDR_RGBA32_hex_string)
{
    ASSERT_EQ(to_html_string_rgba(Color::red()), "#ff0000ff");
    ASSERT_EQ(to_html_string_rgba(Color::green()), "#00ff00ff");
    ASSERT_EQ(to_html_string_rgba(Color::blue()), "#0000ffff");
    ASSERT_EQ(to_html_string_rgba(Color::black()), "#000000ff");
    ASSERT_EQ(to_html_string_rgba(Color::clear()), "#00000000");
    ASSERT_EQ(to_html_string_rgba(Color::white()), "#ffffffff");
    ASSERT_EQ(to_html_string_rgba(Color::yellow()), "#ffff00ff");
    ASSERT_EQ(to_html_string_rgba(Color::cyan()), "#00ffffff");
    ASSERT_EQ(to_html_string_rgba(Color::magenta()), "#ff00ffff");

    // ... and HDR values are LDR clamped
    ASSERT_EQ(to_html_string_rgba(Color(1.5f, 1.5f, 0.0f, 1.0f)), "#ffff00ff");

    // ... and negative values are clamped
    ASSERT_EQ(to_html_string_rgba(Color(-1.0f, 0.0f, 0.0f, 1.0f)), "#000000ff");
}

TEST(Color, try_parse_html_color_string_parses_LDR_RGBx32_hex_string_to_Color)
{
    // when caller specifies all components (incl. alpha)
    ASSERT_EQ(try_parse_html_color_string("#ff0000ff"), Color::red());
    ASSERT_EQ(try_parse_html_color_string("#00ff00ff"), Color::green());
    ASSERT_EQ(try_parse_html_color_string("#0000ffff"), Color::blue());
    ASSERT_EQ(try_parse_html_color_string("#000000ff"), Color::black());
    ASSERT_EQ(try_parse_html_color_string("#ffff00ff"), Color::yellow());
    ASSERT_EQ(try_parse_html_color_string("#00000000"), Color::clear());

    // no colorspace conversion occurs on intermediate values (e.g. no sRGB-to-linear)
    ASSERT_EQ(try_parse_html_color_string("#110000ff"), Color((1.0f*16.0f + 1.0f)/255.0f, 0.0f, 0.0f, 1.0f));

    // when caller specifies 3 components, assume alpha == 1.0
    ASSERT_EQ(try_parse_html_color_string("#ff0000"), Color::red());
    ASSERT_EQ(try_parse_html_color_string("#000000"), Color::black());

    // unparseable input
    ASSERT_EQ(try_parse_html_color_string("not a color"), std::nullopt);
    ASSERT_EQ(try_parse_html_color_string(" #ff0000ff"), std::nullopt);  // caller handles whitespace
    ASSERT_EQ(try_parse_html_color_string("ff0000ff"), std::nullopt);  // caller must put the # prefix before the string
    ASSERT_EQ(try_parse_html_color_string("red"), std::nullopt);  // literal hdr_color strings (e.g. as in Unity) aren't supported (yet)
}

TEST(Color, to_ColorHSLA_color_works_as_expected)
{
    for (const auto& [rgba, expected] : c_RGBA_to_HSLA_known_conversion_values) {
        const auto got = to<ColorHSLA>(rgba);
        ASSERT_NEAR(got.hue, expected.hue/360.0f, c_HLSL_conversion_tolerance_per_component);
        ASSERT_NEAR(got.saturation, expected.saturation, c_HLSL_conversion_tolerance_per_component);
        ASSERT_NEAR(got.lightness, expected.lightness, c_HLSL_conversion_tolerance_per_component);
        ASSERT_NEAR(got.alpha, expected.alpha, c_HLSL_conversion_tolerance_per_component);
    }
}

TEST(Color, hsla_color_to_Color_works_as_expected)
{
    for (const auto& tc : c_RGBA_to_HSLA_known_conversion_values) {
        auto normalized = tc.expected_output;
        normalized.hue /= 360.0f;

        const auto got = to<Color>(normalized);
        ASSERT_NEAR(got.r, tc.input.r, c_HLSL_conversion_tolerance_per_component) << tc << ", got = " << got;
        ASSERT_NEAR(got.g, tc.input.g, c_HLSL_conversion_tolerance_per_component) << tc << ", got = " << got;
        ASSERT_NEAR(got.b, tc.input.b, c_HLSL_conversion_tolerance_per_component) << tc << ", got = " << got;
        ASSERT_NEAR(got.a, tc.input.a, c_HLSL_conversion_tolerance_per_component) << tc << ", got = " << got;
    }
}

TEST(Color, with_element_works_as_expected)
{
    ASSERT_EQ(Color::black().with_element(0, 1.0f), Color::red());
    ASSERT_EQ(Color::black().with_element(1, 1.0f), Color::green());
    ASSERT_EQ(Color::black().with_element(2, 1.0f), Color::blue());
    ASSERT_EQ(Color::clear().with_element(3, 0.5f), Color(0.0f, 0.5f));
}

#include <oscar/Graphics/Texture2D.h>

#include <testoscar/TestingHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Color32.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/TextureFilterMode.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Graphics/TextureWrapMode.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/ObjectRepresentation.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <sstream>
#include <utility>
#include <vector>

using namespace osc::testing;
using namespace osc;
namespace rgs = std::ranges;

namespace
{
    Texture2D generate_2x2_texture()
    {
        Texture2D rv{Vec2i{2, 2}};
        rv.set_pixels(std::vector<Color>(4, Color::red()));
        return rv;
    }
}

TEST(Texture2D, constructor_throws_if_given_zero_or_negatively_sized_dimensions)
{
    ASSERT_ANY_THROW({ Texture2D(Vec2i(0, 0)); });   // x and y are zero
    ASSERT_ANY_THROW({ Texture2D(Vec2i(0, 1)); });   // x is zero
    ASSERT_ANY_THROW({ Texture2D(Vec2i(1, 0)); });   // y is zero

    ASSERT_ANY_THROW({ Texture2D(Vec2i(-1, -1)); }); // x any y are negative
    ASSERT_ANY_THROW({ Texture2D(Vec2i(-1, 1)); });  // x is negative
    ASSERT_ANY_THROW({ Texture2D(Vec2i(1, -1)); });  // y is negative
}

TEST(Texture2D, default_constructor_creates_sRGBA_texture_with_expected_params)
{
    const Texture2D texture_2d{Vec2i(1, 1)};

    ASSERT_EQ(texture_2d.dimensions(), Vec2i(1, 1));
    ASSERT_EQ(texture_2d.texture_format(), TextureFormat::RGBA32);
    ASSERT_EQ(texture_2d.color_space(), ColorSpace::sRGB);
    ASSERT_EQ(texture_2d.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(texture_2d.filter_mode(), TextureFilterMode::Linear);
}

TEST(Texture2D, can_set_pixels32_on_default_constructed_instance)
{
    const Vec2i dimensions = {1, 1};
    const std::vector<Color32> pixels(static_cast<size_t>(dimensions.x * dimensions.y));

    Texture2D texture_2d{dimensions};
    texture_2d.set_pixels32(pixels);

    ASSERT_EQ(texture_2d.dimensions(), dimensions);
    ASSERT_EQ(texture_2d.pixels32(), pixels);
}

TEST(Texture2D, set_pixels_throws_if_number_of_pixels_does_not_match_dimensions)
{
    const Vec2i dimensions = {1, 1};
    const std::vector<Color> incorrect_number_of_pixels(dimensions.x * dimensions.y + 1);

    Texture2D texture_2d{dimensions};

    ASSERT_ANY_THROW({ texture_2d.set_pixels(incorrect_number_of_pixels); });
}

TEST(Texture2D, set_pixels32_throws_if_number_of_pixels_does_not_match_dimensions)
{
    const Vec2i dimensions = {1, 1};
    const std::vector<Color32> incorrect_number_of_pixels(dimensions.x * dimensions.y + 1);

    Texture2D texture_2d{dimensions};
    ASSERT_ANY_THROW({ texture_2d.set_pixels32(incorrect_number_of_pixels); });
}

TEST(Texture2D, set_pixel_data_throws_if_number_of_bytes_occupied_by_pixels_does_not_match_dimensions_and_format_of_Texture2D)
{
    const Vec2i dimensions = {1, 1};
    const std::vector<Color32> incorrect_number_of_pixels(dimensions.x * dimensions.y + 1);

    Texture2D texture_2d{dimensions};

    ASSERT_EQ(texture_2d.texture_format(), TextureFormat::RGBA32);  // sanity check
    ASSERT_ANY_THROW({ texture_2d.set_pixel_data(view_object_representations<uint8_t>(incorrect_number_of_pixels)); });
}

TEST(Texture2D, set_pixel_data_does_not_throw_if_given_valid_number_of_pixel_bytes)
{
    const Vec2i dimensions = {1, 1};
    const std::vector<Color32> pixels(static_cast<size_t>(dimensions.x * dimensions.y));

    Texture2D texture_2d{dimensions};

    ASSERT_EQ(texture_2d.texture_format(), TextureFormat::RGBA32);  // sanity check

    texture_2d.set_pixel_data(view_object_representations<uint8_t>(pixels));
}

TEST(Texture2D, set_pixel_data_works_fine_for_8_bit_single_component_data)
{
    const Vec2i dimensions = {1, 1};
    const std::vector<uint8_t> single_component_pixels(static_cast<size_t>(dimensions.x * dimensions.y));

    Texture2D texture_2d{dimensions, TextureFormat::R8};
    texture_2d.set_pixel_data(single_component_pixels);  // shouldn't throw
}

TEST(Texture2D, set_pixel_data_with_8_bit_single_component_data_followed_by_get_pixels_zeroes_out_green_and_red)
{
    const uint8_t color_uint8{0x88};
    const float color_float = static_cast<float>(color_uint8) / 255.0f;
    const Vec2i dimensions = {1, 1};
    const std::vector<uint8_t> single_component_pixels(static_cast<size_t>(dimensions.x * dimensions.y), color_uint8);

    Texture2D texture_2d{dimensions, TextureFormat::R8};
    texture_2d.set_pixel_data(single_component_pixels);

    for (const Color& pixel : texture_2d.pixels()) {
        ASSERT_EQ(pixel, Color(color_float, 0.0f, 0.0f, 1.0f));
    }
}

TEST(Texture2D, set_pixel_data_with_8_bit_single_component_data_followed_by_get_pixels32_zeroes_out_green_and_red)
{
    const uint8_t color{0x88};
    const Vec2i dimensions = {1, 1};
    const std::vector<uint8_t> single_component_pixels(static_cast<size_t>(dimensions.x * dimensions.y), color);

    Texture2D texture_2d{dimensions, TextureFormat::R8};
    texture_2d.set_pixel_data(single_component_pixels);

    for (const Color32& pixel : texture_2d.pixels32()) {
        const Color32 expected{color, 0x00, 0x00, 0xff};
        ASSERT_EQ(pixel, expected);
    }
}

TEST(Texture2D, set_pixel_data_with_32bit_floating_point_components_followed_by_get_pixels_returns_same_span)
{
    const Vec4 color = generate<Vec4>();
    const Vec2i dimensions = {1, 1};
    std::vector<Vec4> const rgba_float32_pixels(static_cast<size_t>(dimensions.x * dimensions.y), color);

    Texture2D texture_2d(dimensions, TextureFormat::RGBAFloat);  // note: the format matches the incoming data
    texture_2d.set_pixel_data(view_object_representations<uint8_t>(rgba_float32_pixels));

    ASSERT_TRUE(rgs::equal(texture_2d.pixel_data(), view_object_representations<uint8_t>(rgba_float32_pixels)));
}

TEST(Texture2D, set_pixel_data_with_32bit_HDR_floating_point_components_followed_by_get_pixels_returns_same_values)
{
    const Color hdr_color = {1.2f, 1.4f, 1.3f, 1.0f};
    const Vec2i dimensions = {1, 1};
    const std::vector<Color> rgba_hdr_pixels(static_cast<size_t>(dimensions.x * dimensions.y), hdr_color);

    Texture2D texture_2d(dimensions, TextureFormat::RGBAFloat);
    texture_2d.set_pixel_data(view_object_representations<uint8_t>(rgba_hdr_pixels));

    ASSERT_EQ(texture_2d.pixels(), rgba_hdr_pixels);  // because the texture holds 32-bit floats
}

TEST(Texture2D, set_pixel_data_on_8bit_component_format_clamps_hdr_color_values)
{
    const Color hdr_color = {1.2f, 1.4f, 1.3f, 1.0f};
    const Vec2i dimensions = {1, 1};
    const std::vector<Color> hdr_pixels(static_cast<size_t>(dimensions.x * dimensions.y), hdr_color);

    Texture2D texture_2d(dimensions, TextureFormat::RGBA32);  // note: not a HDR-capable format
    texture_2d.set_pixels(hdr_pixels);

    ASSERT_NE(texture_2d.pixels(), hdr_pixels);  // because the impl had to convert them
}

TEST(Texture2D, set_pixels32_on_an_8bit_texture_performs_no_conversion)
{
    const Color32 color32 = {0x77, 0x63, 0x24, 0x76};
    const Vec2i dimensions = {1, 1};
    const std::vector<Color32> pixels32(static_cast<size_t>(dimensions.x * dimensions.y), color32);

    Texture2D texture_2d(dimensions, TextureFormat::RGBA32);  // note: matches pixel format
    texture_2d.set_pixels32(pixels32);

    ASSERT_EQ(texture_2d.pixels32(), pixels32);  // because no conversion was required
}

TEST(Texture2D, set_pixels32_on_32bit_texture_doesnt_observibly_change_component_values)
{
    const Color32 color32 = {0x77, 0x63, 0x24, 0x76};
    const Vec2i dimensions = {1, 1};
    const std::vector<Color32> pixels32(static_cast<size_t>(dimensions.x * dimensions.y), color32);

    Texture2D texture_2d(dimensions, TextureFormat::RGBAFloat);  // note: higher precision than input
    texture_2d.set_pixels32(pixels32);

    ASSERT_EQ(texture_2d.pixels32(), pixels32);  // because, although conversion happened, it was _from_ a higher precision
}

TEST(Texture2D, can_copy_construct)
{
    const Texture2D texture_2d = generate_2x2_texture();
    const Texture2D copy{texture_2d};  // NOLINT(performance-unnecessary-copy-initialization)
}

TEST(Texture2D, can_move_construct)
{
    Texture2D texture_2d = generate_2x2_texture();
    const Texture2D copy{std::move(texture_2d)};
}

TEST(Texture2D, can_copy_assign)
{
    Texture2D lhs = generate_2x2_texture();
    const Texture2D rhs = generate_2x2_texture();

    lhs = rhs;
}

TEST(Texture2D, can_move_assign)
{
    Texture2D lhs = generate_2x2_texture();
    Texture2D rhs = generate_2x2_texture();

    lhs = std::move(rhs);
}

TEST(Texture2D, dimensions_x_returns_the_width_supplied_via_the_constructor)
{
    const int width = 2;
    const int height = 6;

    const Texture2D texture_2d{{width, height}};

    ASSERT_EQ(texture_2d.dimensions().x, width);
}

TEST(Texture2D, dimensions_y_returns_the_height_supplied_via_the_constructor)
{
    const int width = 2;
    const int height = 6;
    const Texture2D texture_2d{{width, height}};

    ASSERT_EQ(texture_2d.dimensions().y, height);
}

TEST(Texture2D, color_space_returns_color_space_provided_via_the_constructor)
{
    for (const ColorSpace color_space : {ColorSpace::sRGB, ColorSpace::Linear}) {
        const Texture2D texture_2d{{1, 1}, TextureFormat::RGBA32, color_space};
        ASSERT_EQ(texture_2d.color_space(), color_space);
    }
}

TEST(Texture2D, wrap_mode_returns_Repeat_on_default_constructed_instance)
{
    const Texture2D texture_2d = generate_2x2_texture();

    ASSERT_EQ(texture_2d.wrap_mode(), TextureWrapMode::Repeat);
}

TEST(Texture2D, set_wrap_mode_makes_wrap_mode_return_new_wrap_mode)
{
    Texture2D texture_2d = generate_2x2_texture();

    const TextureWrapMode wrap_mode = TextureWrapMode::Mirror;

    ASSERT_NE(texture_2d.wrap_mode(), wrap_mode);
    texture_2d.set_wrap_mode(wrap_mode);
    ASSERT_EQ(texture_2d.wrap_mode(), wrap_mode);
}

TEST(Texture2D, set_wrap_mode_causes_wrap_mode_u_to_return_new_wrap_mode)
{
    Texture2D texture_2d = generate_2x2_texture();

    const TextureWrapMode wrap_mode = TextureWrapMode::Mirror;

    ASSERT_NE(texture_2d.wrap_mode(), wrap_mode);
    ASSERT_NE(texture_2d.wrap_mode_u(), wrap_mode);
    texture_2d.set_wrap_mode(wrap_mode);
    ASSERT_EQ(texture_2d.wrap_mode_u(), wrap_mode);
}

TEST(Texture2D, set_wrap_mode_u_causes_wrap_mode_u_to_return_wrap_mode)
{
    Texture2D texture_2d = generate_2x2_texture();

    const TextureWrapMode wrap_mode = TextureWrapMode::Mirror;

    ASSERT_NE(texture_2d.wrap_mode_u(), wrap_mode);
    texture_2d.set_wrap_mode_u(wrap_mode);
    ASSERT_EQ(texture_2d.wrap_mode_u(), wrap_mode);
}

TEST(Texture2D, set_wrap_mode_v_causes_wrap_mode_v_to_return_wrap_mode)
{
    Texture2D texture_2d = generate_2x2_texture();

    const TextureWrapMode wrap_mode = TextureWrapMode::Mirror;

    ASSERT_NE(texture_2d.wrap_mode_v(), wrap_mode);
    texture_2d.set_wrap_mode_v(wrap_mode);
    ASSERT_EQ(texture_2d.wrap_mode_v(), wrap_mode);
}

TEST(Texture2D, set_wrap_mode_w_causes_wrap_mode_w_to_return_wrap_mode)
{
    Texture2D texture_2d = generate_2x2_texture();

    const TextureWrapMode wrap_mode = TextureWrapMode::Mirror;

    ASSERT_NE(texture_2d.wrap_mode_w(), wrap_mode);
    texture_2d.set_wrap_mode_w(wrap_mode);
    ASSERT_EQ(texture_2d.wrap_mode_w(), wrap_mode);
}

TEST(Texture2D, set_filter_mode_causes_filter_mode_to_return_filter_mode)
{
    Texture2D texture_2d = generate_2x2_texture();

    const TextureFilterMode filter_mode = TextureFilterMode::Nearest;

    ASSERT_NE(texture_2d.filter_mode(), filter_mode);
    texture_2d.set_filter_mode(filter_mode);
    ASSERT_EQ(texture_2d.filter_mode(), filter_mode);
}

TEST(Texture2D, set_filter_mode_returns_Mipmap_when_set)
{
    Texture2D texture_2d = generate_2x2_texture();

    const TextureFilterMode filter_mode = TextureFilterMode::Mipmap;

    ASSERT_NE(texture_2d.filter_mode(), filter_mode);
    texture_2d.set_filter_mode(filter_mode);
    ASSERT_EQ(texture_2d.filter_mode(), filter_mode);
}

TEST(Texture2D, is_equality_comparable)
{
    const Texture2D lhs = generate_2x2_texture();
    const Texture2D rhs = generate_2x2_texture();

    (void)(lhs == rhs);  // just ensure it compiles + runs
}

TEST(Texture2D, compares_equal_to_copy_constructed_instance)
{
    const Texture2D texture_2d = generate_2x2_texture();
    const Texture2D copy_constructed{texture_2d};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(texture_2d, copy_constructed);
}

TEST(Texture2D, compares_equal_to_copy_assigned_instance)
{
    Texture2D lhs = generate_2x2_texture();
    const Texture2D rhs = generate_2x2_texture();

    lhs = rhs;
    ASSERT_EQ(lhs, rhs);
}

TEST(Texture2D, not_equals_operator_is_available)
{
    const Texture2D lhs = generate_2x2_texture();
    const Texture2D rhs = generate_2x2_texture();

    (void)(lhs != rhs);  // just ensure this expression compiles
}

TEST(Texture2D, set_wrap_mode_makes_an_equal_copy_compare_not_equal)
{
    const Texture2D texture_2d = generate_2x2_texture();
    Texture2D copy_constructed{texture_2d};
    const TextureWrapMode wrap_mode = TextureWrapMode::Clamp;

    ASSERT_EQ(texture_2d, copy_constructed);
    ASSERT_NE(copy_constructed.wrap_mode(), wrap_mode);
    copy_constructed.set_wrap_mode(wrap_mode);
    ASSERT_NE(texture_2d, copy_constructed);
}

TEST(Texture2D, set_wrap_mode_u_makes_an_equal_copy_compare_not_equal)
{
    const Texture2D texture_2d = generate_2x2_texture();
    Texture2D copy_constructed{texture_2d};
    const TextureWrapMode wrap_mode = TextureWrapMode::Clamp;

    ASSERT_EQ(texture_2d, copy_constructed);
    ASSERT_NE(copy_constructed.wrap_mode_u(), wrap_mode);
    copy_constructed.set_wrap_mode_u(wrap_mode);
    ASSERT_NE(texture_2d, copy_constructed);
}

TEST(Texture2D, set_wrap_mode_v_makes_an_equal_copy_compare_not_equal)
{
    const Texture2D texture_2d = generate_2x2_texture();
    Texture2D copy_constructed{texture_2d};
    const TextureWrapMode wrap_mode = TextureWrapMode::Clamp;

    ASSERT_EQ(texture_2d, copy_constructed);
    ASSERT_NE(copy_constructed.wrap_mode_v(), wrap_mode);
    copy_constructed.set_wrap_mode_v(wrap_mode);
    ASSERT_NE(texture_2d, copy_constructed);
}

TEST(Texture2D, set_wrap_mode_w_makes_an_equal_copy_compare_not_equal)
{
    const Texture2D texture_2d = generate_2x2_texture();
    Texture2D copy_constructed{texture_2d};
    const TextureWrapMode wrap_mode = TextureWrapMode::Clamp;

    ASSERT_EQ(texture_2d, copy_constructed);
    ASSERT_NE(copy_constructed.wrap_mode_w(), wrap_mode);
    copy_constructed.set_wrap_mode_w(wrap_mode);
    ASSERT_NE(texture_2d, copy_constructed);
}

TEST(Texture2D, set_filter_mode_makes_an_equal_copy_compare_not_equal)
{
    const Texture2D texture_2d = generate_2x2_texture();
    Texture2D copy_constructed{texture_2d};
    const TextureFilterMode filter_mode = TextureFilterMode::Nearest;

    ASSERT_EQ(texture_2d, copy_constructed);
    ASSERT_NE(copy_constructed.filter_mode(), filter_mode);
    copy_constructed.set_filter_mode(filter_mode);
    ASSERT_NE(texture_2d, copy_constructed);
}

TEST(Texture2D, can_be_written_to_a_std_ostream)
{
    const Texture2D texture_2d = generate_2x2_texture();

    std::stringstream ss;
    ss << texture_2d;

    ASSERT_FALSE(ss.str().empty());
}

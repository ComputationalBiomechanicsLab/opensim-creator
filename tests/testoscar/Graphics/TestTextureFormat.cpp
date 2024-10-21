#include <oscar/Graphics/TextureFormat.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/TextureComponentFormat.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstdint>
#include <optional>
#include <type_traits>

using namespace osc;

static_assert(std::is_same_v<std::underlying_type_t<TextureFormat>, int32_t>);

TEST(TextureFormat, num_components_in_returns_expected_values)
{
    static_assert(num_options<TextureFormat>() == 7);

    ASSERT_EQ(num_components_in(TextureFormat::R8), 1);
    ASSERT_EQ(num_components_in(TextureFormat::RG16), 2);
    ASSERT_EQ(num_components_in(TextureFormat::RGB24), 3);
    ASSERT_EQ(num_components_in(TextureFormat::RGBA32), 4);
    ASSERT_EQ(num_components_in(TextureFormat::RGFloat), 2);
    ASSERT_EQ(num_components_in(TextureFormat::RGBFloat), 3);
    ASSERT_EQ(num_components_in(TextureFormat::RGBAFloat), 4);
}

TEST(TextureFormat, component_format_of_returns_expected_values)
{
    static_assert(num_options<TextureFormat>() == 7);

    ASSERT_EQ(component_format_of(TextureFormat::R8), TextureComponentFormat::Uint8);
    ASSERT_EQ(component_format_of(TextureFormat::RG16), TextureComponentFormat::Uint8);
    ASSERT_EQ(component_format_of(TextureFormat::RGB24), TextureComponentFormat::Uint8);
    ASSERT_EQ(component_format_of(TextureFormat::RGBA32), TextureComponentFormat::Uint8);
    ASSERT_EQ(component_format_of(TextureFormat::RGFloat), TextureComponentFormat::Float32);
    ASSERT_EQ(component_format_of(TextureFormat::RGBFloat), TextureComponentFormat::Float32);
    ASSERT_EQ(component_format_of(TextureFormat::RGBAFloat), TextureComponentFormat::Float32);
}

TEST(TextureFormat, num_bytes_per_pixel_in_returns_expected_values)
{
    static_assert(num_options<TextureFormat>() == 7);

    ASSERT_EQ(num_bytes_per_pixel_in(TextureFormat::R8), 1);
    ASSERT_EQ(num_bytes_per_pixel_in(TextureFormat::RG16), 2);
    ASSERT_EQ(num_bytes_per_pixel_in(TextureFormat::RGB24), 3);
    ASSERT_EQ(num_bytes_per_pixel_in(TextureFormat::RGBA32), 4);
    ASSERT_EQ(num_bytes_per_pixel_in(TextureFormat::RGFloat), 8);
    ASSERT_EQ(num_bytes_per_pixel_in(TextureFormat::RGBFloat), 12);
    ASSERT_EQ(num_bytes_per_pixel_in(TextureFormat::RGBAFloat), 16);
}

TEST(TextureFormat, to_texture_format_returns_expected_values)
{
    static_assert(num_options<TextureFormat>() == 7);

    ASSERT_EQ(to_texture_format(1, TextureComponentFormat::Uint8), TextureFormat::R8);
    ASSERT_EQ(to_texture_format(2, TextureComponentFormat::Uint8), TextureFormat::RG16);
    ASSERT_EQ(to_texture_format(3, TextureComponentFormat::Uint8), TextureFormat::RGB24);
    ASSERT_EQ(to_texture_format(4, TextureComponentFormat::Uint8), TextureFormat::RGBA32);
    ASSERT_EQ(to_texture_format(5, TextureComponentFormat::Uint8), std::nullopt);

    ASSERT_EQ(to_texture_format(1, TextureComponentFormat::Float32), std::nullopt);
    ASSERT_EQ(to_texture_format(2, TextureComponentFormat::Float32), TextureFormat::RGFloat);
    ASSERT_EQ(to_texture_format(3, TextureComponentFormat::Float32), TextureFormat::RGBFloat);
    ASSERT_EQ(to_texture_format(4, TextureComponentFormat::Float32), TextureFormat::RGBAFloat);
    ASSERT_EQ(to_texture_format(5, TextureComponentFormat::Float32), std::nullopt);
}

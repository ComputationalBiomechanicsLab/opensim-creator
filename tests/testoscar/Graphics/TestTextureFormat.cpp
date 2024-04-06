#include <oscar/Graphics/TextureFormat.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/TextureChannelFormat.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstdint>
#include <optional>
#include <type_traits>

using namespace osc;

static_assert(std::is_same_v<std::underlying_type_t<TextureFormat>, int32_t>);

TEST(TextureFormat, NumChannelsReturnsExpectedValues)
{
    static_assert(num_options<TextureFormat>() == 7);

    ASSERT_EQ(num_channels_in(TextureFormat::R8), 1);
    ASSERT_EQ(num_channels_in(TextureFormat::RG16), 2);
    ASSERT_EQ(num_channels_in(TextureFormat::RGB24), 3);
    ASSERT_EQ(num_channels_in(TextureFormat::RGBA32), 4);
    ASSERT_EQ(num_channels_in(TextureFormat::RGFloat), 2);
    ASSERT_EQ(num_channels_in(TextureFormat::RGBFloat), 3);
    ASSERT_EQ(num_channels_in(TextureFormat::RGBAFloat), 4);
}

TEST(TextureFormat, ChannelFormatReturnsExpectedValues)
{
    static_assert(num_options<TextureFormat>() == 7);

    ASSERT_EQ(channel_format_of(TextureFormat::R8), TextureChannelFormat::Uint8);
    ASSERT_EQ(channel_format_of(TextureFormat::RG16), TextureChannelFormat::Uint8);
    ASSERT_EQ(channel_format_of(TextureFormat::RGB24), TextureChannelFormat::Uint8);
    ASSERT_EQ(channel_format_of(TextureFormat::RGBA32), TextureChannelFormat::Uint8);
    ASSERT_EQ(channel_format_of(TextureFormat::RGFloat), TextureChannelFormat::Float32);
    ASSERT_EQ(channel_format_of(TextureFormat::RGBFloat), TextureChannelFormat::Float32);
    ASSERT_EQ(channel_format_of(TextureFormat::RGBAFloat), TextureChannelFormat::Float32);
}

TEST(TextureFormat, NumBytesPerPixelReturnsExpectedValues)
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

TEST(TextureFormat, ToTextureFormatReturnsExpectedValues)
{
    static_assert(num_options<TextureFormat>() == 7);

    ASSERT_EQ(to_texture_format(1, TextureChannelFormat::Uint8), TextureFormat::R8);
    ASSERT_EQ(to_texture_format(2, TextureChannelFormat::Uint8), TextureFormat::RG16);
    ASSERT_EQ(to_texture_format(3, TextureChannelFormat::Uint8), TextureFormat::RGB24);
    ASSERT_EQ(to_texture_format(4, TextureChannelFormat::Uint8), TextureFormat::RGBA32);
    ASSERT_EQ(to_texture_format(5, TextureChannelFormat::Uint8), std::nullopt);

    ASSERT_EQ(to_texture_format(1, TextureChannelFormat::Float32), std::nullopt);
    ASSERT_EQ(to_texture_format(2, TextureChannelFormat::Float32), TextureFormat::RGFloat);
    ASSERT_EQ(to_texture_format(3, TextureChannelFormat::Float32), TextureFormat::RGBFloat);
    ASSERT_EQ(to_texture_format(4, TextureChannelFormat::Float32), TextureFormat::RGBAFloat);
    ASSERT_EQ(to_texture_format(5, TextureChannelFormat::Float32), std::nullopt);
}

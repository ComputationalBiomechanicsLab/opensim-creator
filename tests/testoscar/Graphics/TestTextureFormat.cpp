#include <oscar/Graphics/TextureFormat.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/TextureChannelFormat.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <cstdint>
#include <optional>
#include <type_traits>

using osc::NumChannels;
using osc::NumOptions;
using osc::TextureChannelFormat;
using osc::TextureFormat;

static_assert(std::is_same_v<std::underlying_type_t<TextureFormat>, int32_t>);

TEST(TextureFormat, NumChannelsReturnsExpectedValues)
{
    static_assert(NumOptions<TextureFormat>() == 7);

    ASSERT_EQ(NumChannels(TextureFormat::R8), 1);
    ASSERT_EQ(NumChannels(TextureFormat::RG16), 2);
    ASSERT_EQ(NumChannels(TextureFormat::RGB24), 3);
    ASSERT_EQ(NumChannels(TextureFormat::RGBA32), 4);
    ASSERT_EQ(NumChannels(TextureFormat::RGFloat), 2);
    ASSERT_EQ(NumChannels(TextureFormat::RGBFloat), 3);
    ASSERT_EQ(NumChannels(TextureFormat::RGBAFloat), 4);
}

TEST(TextureFormat, ChannelFormatReturnsExpectedValues)
{
    static_assert(NumOptions<TextureFormat>() == 7);

    ASSERT_EQ(ChannelFormat(TextureFormat::R8), TextureChannelFormat::Uint8);
    ASSERT_EQ(ChannelFormat(TextureFormat::RG16), TextureChannelFormat::Uint8);
    ASSERT_EQ(ChannelFormat(TextureFormat::RGB24), TextureChannelFormat::Uint8);
    ASSERT_EQ(ChannelFormat(TextureFormat::RGBA32), TextureChannelFormat::Uint8);
    ASSERT_EQ(ChannelFormat(TextureFormat::RGFloat), TextureChannelFormat::Float32);
    ASSERT_EQ(ChannelFormat(TextureFormat::RGBFloat), TextureChannelFormat::Float32);
    ASSERT_EQ(ChannelFormat(TextureFormat::RGBAFloat), TextureChannelFormat::Float32);
}

TEST(TextureFormat, NumBytesPerPixelReturnsExpectedValues)
{
    static_assert(NumOptions<TextureFormat>() == 7);

    ASSERT_EQ(NumBytesPerPixel(TextureFormat::R8), 1);
    ASSERT_EQ(NumBytesPerPixel(TextureFormat::RG16), 2);
    ASSERT_EQ(NumBytesPerPixel(TextureFormat::RGB24), 3);
    ASSERT_EQ(NumBytesPerPixel(TextureFormat::RGBA32), 4);
    ASSERT_EQ(NumBytesPerPixel(TextureFormat::RGFloat), 8);
    ASSERT_EQ(NumBytesPerPixel(TextureFormat::RGBFloat), 12);
    ASSERT_EQ(NumBytesPerPixel(TextureFormat::RGBAFloat), 16);
}

TEST(TextureFormat, ToTextureFormatReturnsExpectedValues)
{
    static_assert(NumOptions<TextureFormat>() == 7);

    ASSERT_EQ(ToTextureFormat(1, TextureChannelFormat::Uint8), TextureFormat::R8);
    ASSERT_EQ(ToTextureFormat(2, TextureChannelFormat::Uint8), TextureFormat::RG16);
    ASSERT_EQ(ToTextureFormat(3, TextureChannelFormat::Uint8), TextureFormat::RGB24);
    ASSERT_EQ(ToTextureFormat(4, TextureChannelFormat::Uint8), TextureFormat::RGBA32);
    ASSERT_EQ(ToTextureFormat(5, TextureChannelFormat::Uint8), std::nullopt);

    ASSERT_EQ(ToTextureFormat(1, TextureChannelFormat::Float32), std::nullopt);
    ASSERT_EQ(ToTextureFormat(2, TextureChannelFormat::Float32), TextureFormat::RGFloat);
    ASSERT_EQ(ToTextureFormat(3, TextureChannelFormat::Float32), TextureFormat::RGBFloat);
    ASSERT_EQ(ToTextureFormat(4, TextureChannelFormat::Float32), TextureFormat::RGBAFloat);
    ASSERT_EQ(ToTextureFormat(5, TextureChannelFormat::Float32), std::nullopt);
}

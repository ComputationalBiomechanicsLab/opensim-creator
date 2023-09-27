#include "oscar/Graphics/TextureFormat.hpp"

#include "oscar/Graphics/TextureChannelFormat.hpp"
#include "oscar/Utils/EnumHelpers.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <type_traits>

static_assert(std::is_same_v<std::underlying_type_t<osc::TextureFormat>, int32_t>);

TEST(TextureFormat, NumChannelsReturnsExpectedValues)
{
    static_assert(osc::NumOptions<osc::TextureFormat>() == 5);

    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::R8), 1);
    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::RGB24), 3);
    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::RGBA32), 4);
    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::RGBFloat), 3);
    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::RGBAFloat), 4);
}

TEST(TextureFormat, ChannelFormatReturnsExpectedValues)
{
    static_assert(osc::NumOptions<osc::TextureFormat>() == 5);

    ASSERT_EQ(osc::ChannelFormat(osc::TextureFormat::R8), osc::TextureChannelFormat::Uint8);
    ASSERT_EQ(osc::ChannelFormat(osc::TextureFormat::RGB24), osc::TextureChannelFormat::Uint8);
    ASSERT_EQ(osc::ChannelFormat(osc::TextureFormat::RGBA32), osc::TextureChannelFormat::Uint8);
    ASSERT_EQ(osc::ChannelFormat(osc::TextureFormat::RGBFloat), osc::TextureChannelFormat::Float32);
    ASSERT_EQ(osc::ChannelFormat(osc::TextureFormat::RGBAFloat), osc::TextureChannelFormat::Float32);
}

TEST(TextureFormat, NumBytesPerPixelReturnsExpectedValues)
{
    static_assert(osc::NumOptions<osc::TextureFormat>() == 5);

    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::R8), 1);
    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::RGB24), 3);
    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::RGBA32), 4);
    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::RGBFloat), 12);
    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::RGBAFloat), 16);
}

TEST(TextureFormat, ToTextureFormatReturnsExpectedValues)
{
    static_assert(osc::NumOptions<osc::TextureFormat>() == 5);

    ASSERT_EQ(osc::ToTextureFormat(1, osc::TextureChannelFormat::Uint8), osc::TextureFormat::R8);
    ASSERT_EQ(osc::ToTextureFormat(2, osc::TextureChannelFormat::Uint8), std::nullopt);
    ASSERT_EQ(osc::ToTextureFormat(3, osc::TextureChannelFormat::Uint8), osc::TextureFormat::RGB24);
    ASSERT_EQ(osc::ToTextureFormat(4, osc::TextureChannelFormat::Uint8), osc::TextureFormat::RGBA32);
    ASSERT_EQ(osc::ToTextureFormat(5, osc::TextureChannelFormat::Uint8), std::nullopt);

    ASSERT_EQ(osc::ToTextureFormat(1, osc::TextureChannelFormat::Float32), std::nullopt);
    ASSERT_EQ(osc::ToTextureFormat(2, osc::TextureChannelFormat::Float32), std::nullopt);
    ASSERT_EQ(osc::ToTextureFormat(3, osc::TextureChannelFormat::Float32), osc::TextureFormat::RGBFloat);
    ASSERT_EQ(osc::ToTextureFormat(4, osc::TextureChannelFormat::Float32), osc::TextureFormat::RGBAFloat);
    ASSERT_EQ(osc::ToTextureFormat(5, osc::TextureChannelFormat::Float32), std::nullopt);
}

#include "oscar/Graphics/TextureFormat.hpp"

#include "oscar/Utils/EnumHelpers.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <type_traits>

static_assert(std::is_same_v<std::underlying_type_t<osc::TextureFormat>, int32_t>);

TEST(TextureFormat, NumChannelsReturnsExpectedValues)
{
    static_assert(osc::NumOptions<osc::TextureFormat>() == 4);

    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::R8), 1);
    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::RGB24), 3);
    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::RGBA32), 4);
    ASSERT_EQ(osc::NumChannels(osc::TextureFormat::RGBAFloat), 4);
}

TEST(TextureFormat, NumBytesPerChannelReturnsExpectedValues)
{
    static_assert(osc::NumOptions<osc::TextureFormat>() == 4);

    ASSERT_EQ(osc::NumBytesPerChannel(osc::TextureFormat::R8), 1);
    ASSERT_EQ(osc::NumBytesPerChannel(osc::TextureFormat::RGB24), 1);
    ASSERT_EQ(osc::NumBytesPerChannel(osc::TextureFormat::RGBA32), 1);
    ASSERT_EQ(osc::NumBytesPerChannel(osc::TextureFormat::RGBAFloat), 4);
}

TEST(TextureFormat, NumBytesPerPixelReturnsExpectedValues)
{
    static_assert(osc::NumOptions<osc::TextureFormat>() == 4);

    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::R8), 1);
    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::RGB24), 3);
    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::RGBA32), 4);
    ASSERT_EQ(osc::NumBytesPerPixel(osc::TextureFormat::RGBAFloat), 16);
}

TEST(TextureFormat, ToTextureFormatReturnsExpectedValues)
{
    static_assert(osc::NumOptions<osc::TextureFormat>() == 4);

    ASSERT_EQ(osc::ToTextureFormat<uint8_t>(1), osc::TextureFormat::R8);
    ASSERT_EQ(osc::ToTextureFormat<uint8_t>(2), std::nullopt);
    ASSERT_EQ(osc::ToTextureFormat<uint8_t>(3), osc::TextureFormat::RGB24);
    ASSERT_EQ(osc::ToTextureFormat<uint8_t>(4), osc::TextureFormat::RGBA32);
    ASSERT_EQ(osc::ToTextureFormat<uint8_t>(5), std::nullopt);

    ASSERT_EQ(osc::ToTextureFormat<float>(1), std::nullopt);
    ASSERT_EQ(osc::ToTextureFormat<float>(2), std::nullopt);
    ASSERT_EQ(osc::ToTextureFormat<float>(3), std::nullopt);
    ASSERT_EQ(osc::ToTextureFormat<float>(4), osc::TextureFormat::RGBAFloat);
    ASSERT_EQ(osc::ToTextureFormat<float>(5), std::nullopt);
}
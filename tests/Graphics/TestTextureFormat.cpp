#include "src/Graphics/TextureFormat.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <type_traits>

static_assert(std::is_same_v<std::underlying_type_t<osc::TextureFormat>, int32_t>);

TEST(TextureFormat, NumChannelsAsTextureFormatReturnsExpectedValues)
{
    ASSERT_EQ(osc::NumChannelsAsTextureFormat(-3), std::nullopt);
    ASSERT_EQ(osc::NumChannelsAsTextureFormat(0), std::nullopt);
    ASSERT_EQ(osc::NumChannelsAsTextureFormat(1), osc::TextureFormat::R8);
    ASSERT_EQ(osc::NumChannelsAsTextureFormat(2), std::nullopt);
    ASSERT_EQ(osc::NumChannelsAsTextureFormat(3), osc::TextureFormat::RGB24);
    ASSERT_EQ(osc::NumChannelsAsTextureFormat(4), osc::TextureFormat::RGBA32);
    ASSERT_EQ(osc::NumChannelsAsTextureFormat(5), std::nullopt);
}

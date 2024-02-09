#include <oscar/Graphics/TextureChannelFormat.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

using osc::TextureChannelFormat;
using osc::NumOptions;
using osc::NumBytesPerChannel;

TEST(TextureChannelFormat, NumBytesPerChannelReturnsExpectedValues)
{
    static_assert(NumOptions<TextureChannelFormat>() == 2);

    ASSERT_EQ(NumBytesPerChannel(TextureChannelFormat::Uint8), 1);
    ASSERT_EQ(NumBytesPerChannel(TextureChannelFormat::Float32), 4);
}

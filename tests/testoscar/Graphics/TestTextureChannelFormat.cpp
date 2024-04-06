#include <oscar/Graphics/TextureChannelFormat.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

using namespace osc;

TEST(TextureChannelFormat, NumBytesPerChannelReturnsExpectedValues)
{
    static_assert(num_options<TextureChannelFormat>() == 2);

    ASSERT_EQ(num_bytes_per_channel_in(TextureChannelFormat::Uint8), 1);
    ASSERT_EQ(num_bytes_per_channel_in(TextureChannelFormat::Float32), 4);
}

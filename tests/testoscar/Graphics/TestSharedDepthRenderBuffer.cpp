#include <oscar/Graphics/SharedDepthRenderBuffer.h>

#include <oscar/Graphics/ColorRenderBufferParams.h>
#include <oscar/Graphics/DepthRenderBufferParams.h>
#include <gtest/gtest.h>

using namespace osc;

TEST(SharedDepthRenderBuffer, can_default_construct)
{
    [[maybe_unused]] const SharedDepthRenderBuffer default_constructed;
}

TEST(SharedDepthRenderBuffer, can_construct_depth_buffer)
{
    [[maybe_unused]] const SharedDepthRenderBuffer depth_buffer{DepthRenderBufferParams{}};
}

TEST(SharedDepthRenderBuffer, dimensionality_defaults_to_Tex2D)
{
    ASSERT_EQ(SharedDepthRenderBuffer{}.dimensionality(), TextureDimensionality::Tex2D);
}

TEST(SharedDepthRenderBuffer, dimensionality_is_based_on_parameters)
{
    const SharedDepthRenderBuffer buffer{DepthRenderBufferParams{.dimensionality = TextureDimensionality::Cube}};
    ASSERT_EQ(buffer.dimensionality(), TextureDimensionality::Cube);
}

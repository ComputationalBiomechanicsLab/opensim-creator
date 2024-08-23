#include <oscar/Graphics/SharedColorRenderBuffer.h>

#include <oscar/Graphics/ColorRenderBufferParams.h>
#include <oscar/Graphics/DepthRenderBufferParams.h>
#include <gtest/gtest.h>

using namespace osc;

TEST(SharedColorRenderBuffer, can_default_construct)
{
    [[maybe_unused]] const SharedColorRenderBuffer default_constructed;
}

TEST(SharedColorRenderBuffer, can_construct_depth_buffer)
{
    [[maybe_unused]] const SharedColorRenderBuffer depth_buffer{ColorRenderBufferParams{}};
}

TEST(SharedColorRenderBuffer, dimensionality_defaults_to_Tex2D)
{
    ASSERT_EQ(SharedColorRenderBuffer{}.dimensionality(), TextureDimensionality::Tex2D);
}

TEST(SharedColorRenderBuffer, dimensionality_is_based_on_parameters)
{
    const SharedColorRenderBuffer buffer{ColorRenderBufferParams{.dimensionality = TextureDimensionality::Cube}};
    ASSERT_EQ(buffer.dimensionality(), TextureDimensionality::Cube);
}

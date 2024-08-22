#include <oscar/Graphics/SharedRenderBuffer.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(SharedRenderBuffer, can_default_construct)
{
    [[maybe_unused]] const SharedRenderBuffer default_constructed;
}

TEST(SharedRenderBuffer, can_construct_depth_buffer)
{
    [[maybe_unused]] const SharedRenderBuffer depth_buffer{RenderTextureParams{}, RenderBufferType::Depth};
}

TEST(SharedRenderBuffer, dimensionality_defaults_to_Tex2D)
{
    ASSERT_EQ(SharedRenderBuffer{}.dimensionality(), TextureDimensionality::Tex2D);
}

TEST(SharedRenderBuffer, dimensionality_is_based_on_parameters)
{
    const SharedRenderBuffer buffer{{.dimensionality = TextureDimensionality::Cube}, RenderBufferType::Color};
    ASSERT_EQ(buffer.dimensionality(), TextureDimensionality::Cube);
}

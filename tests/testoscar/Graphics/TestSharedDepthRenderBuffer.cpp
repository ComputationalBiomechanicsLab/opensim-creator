#include <oscar/Graphics/SharedDepthRenderBuffer.h>

#include <oscar/Graphics/ColorRenderBufferParams.h>
#include <oscar/Graphics/DepthRenderBufferParams.h>
#include <gtest/gtest.h>

using namespace osc;

TEST(SharedDepthRenderBuffer, can_default_construct)
{
    [[maybe_unused]] const SharedDepthRenderBuffer default_constructed;
}

TEST(SharedDepthRenderBuffer, default_constructed_as_1x1_dimensions)
{
    ASSERT_EQ(SharedDepthRenderBuffer{}.dimensions(), Vec2i(1, 1));
}

TEST(SharedDepthRenderBuffer, default_constructed_with_1x_anti_aliasing)
{
    ASSERT_EQ(SharedDepthRenderBuffer{}.anti_aliasing_level(), AntiAliasingLevel{1});
}

TEST(SharedDepthRenderBuffer, default_constructed_has_D24_depth_stencil_format)
{
    ASSERT_EQ(SharedDepthRenderBuffer{}.depth_stencil_format(), DepthStencilFormat::D24_UNorm_S8_UInt);
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

TEST(SharedDepthRenderBuffer, dimensions_is_based_on_parameters)
{
    const SharedDepthRenderBuffer buffer{{.dimensions = Vec2i(3, 5)}};
    ASSERT_EQ(buffer.dimensions(), Vec2i(3,5));
}

TEST(SharedDepthRenderBuffer, anti_aliasing_level_is_based_on_parameters)
{
    const SharedDepthRenderBuffer buffer{{.anti_aliasing_level = AntiAliasingLevel{4}}};
    ASSERT_EQ(buffer.anti_aliasing_level(), AntiAliasingLevel{4});
}

TEST(SharedDepthRenderBuffer, depth_stencil_format_is_based_on_parameters)
{
    const SharedDepthRenderBuffer buffer{{.depth_format = DepthStencilFormat::D32_SFloat}};
    ASSERT_EQ(buffer.depth_stencil_format(), DepthStencilFormat::D32_SFloat);
}

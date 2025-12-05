#include "RenderTextureParams.h"

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/ColorRenderBufferFormat.h>
#include <liboscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <liboscar/Graphics/TextureDimensionality.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Utils/StringHelpers.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(RenderTextureParams, can_be_default_constructed)
{
    [[maybe_unused]] const RenderTextureParams params;
}

TEST(RenderTextureParams, can_be_constructed_via_designated_initializer)
{
    [[maybe_unused]] const RenderTextureParams params{.pixel_dimensions = {1, 4}};
}

TEST(RenderTextureParams, can_be_copy_constructed)
{
    const RenderTextureParams params;
    [[maybe_unused]] const RenderTextureParams copy{params};
}

TEST(RenderTextureParams, can_be_copy_assigned)
{
    RenderTextureParams lhs;
    const RenderTextureParams rhs;
    lhs = rhs;
}

TEST(RenderTextureParams, device_pixel_ratio_defaults_to_1)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.device_pixel_ratio, 1.0f);
}

TEST(RenderTextureParams, anti_aliasing_level_defaults_to_1)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.anti_aliasing_level, AntiAliasingLevel{1});
}

TEST(RenderTextureParams, color_format_defaults_to_Default)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.color_format, ColorRenderBufferFormat::Default);
}

TEST(RenderTextureParams, depth_stencil_format_defaults_to_Default)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.depth_stencil_format, DepthStencilRenderBufferFormat::Default);
}

TEST(RenderTextureParams, dimensionality_defaults_to_Tex2D)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.dimensionality, TextureDimensionality::Tex2D);
}

TEST(RenderTextureParams, compares_equivalent_on_copy_construction)
{
    const RenderTextureParams params;
    const RenderTextureParams copy{params};

    ASSERT_EQ(params, copy);
}

TEST(RenderTextureParams, compares_equivalent_when_independently_constructed_with_same_params)
{
    const RenderTextureParams first{.pixel_dimensions = {3, 3}, .device_pixel_ratio = 2.0f, .dimensionality = TextureDimensionality::Cube};
    const RenderTextureParams second{.pixel_dimensions = {3, 3}, .device_pixel_ratio = 2.0f, .dimensionality = TextureDimensionality::Cube};

    ASSERT_EQ(first, second);
}

TEST(RenderTextureParams, can_be_streamed_to_string)
{
    const RenderTextureParams params;
    std::stringstream ss;
    ss << params;

    const std::string str{ss.str()};
    ASSERT_TRUE(contains_case_insensitive(str, "RenderTextureParams"));
}

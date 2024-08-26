#include <oscar/Graphics/RenderTextureParams.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/StringHelpers.h>

using namespace osc;

TEST(RenderTextureParams, can_be_default_constructed)
{
    [[maybe_unused]] const RenderTextureParams params;
}

TEST(RenderTextureParams, can_be_constructed_via_designated_initializer)
{
    [[maybe_unused]] const RenderTextureParams params{.dimensions = {1, 4}};
}

TEST(RenderTextureParams, can_be_copy_constructed)
{
    const RenderTextureParams params;
    [[maybe_unused]] const RenderTextureParams copy{params};
}

TEST(RenderTextureDescriptor, can_be_copy_assigned)
{
    RenderTextureParams lhs;
    const RenderTextureParams rhs;
    lhs = rhs;
}

TEST(RenderTextureParams, anti_aliasing_level_defaults_to_1)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.anti_aliasing_level, AntiAliasingLevel{1});
}

TEST(RenderTextureDescriptor, color_format_defaults_to_ARGB32)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.color_format, RenderTextureFormat::ARGB32);
}

TEST(RenderTextureParams, depth_stencil_format_defaults_to_Default)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.depth_stencil_format, DepthStencilFormat::Default);
}

TEST(RenderTextureParams, read_write_defaults_to_Default)
{
    const RenderTextureParams params;
    ASSERT_EQ(params.read_write, RenderTextureReadWrite::Default);
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
    const RenderTextureParams first{.dimensions = {3, 3}, .dimensionality = TextureDimensionality::Cube};
    const RenderTextureParams second{.dimensions = {3, 3}, .dimensionality = TextureDimensionality::Cube};

    ASSERT_EQ(first, second);
}

TEST(RenderTextureParams, can_be_streamed_to_string)
{
    RenderTextureParams params;
    std::stringstream ss;
    ss << params;

    std::string str{ss.str()};
    ASSERT_TRUE(contains_case_insensitive(str, "RenderTextureParams"));
}

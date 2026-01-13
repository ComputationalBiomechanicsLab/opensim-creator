#include "RenderTexture.h"

#include <liboscar/graphics/AntiAliasingLevel.h>
#include <liboscar/graphics/ColorRenderBufferFormat.h>
#include <liboscar/graphics/DepthStencilRenderBufferFormat.h>
#include <liboscar/graphics/RenderTextureParams.h>
#include <liboscar/graphics/TextureDimensionality.h>
#include <liboscar/maths/Vector2.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(RenderTexture, default_constructor_creates_1x1_default_texture)
{
    const RenderTexture render_texture;
    ASSERT_EQ(render_texture.dimensions(), Vector2(1.0f, 1.0f));
    ASSERT_EQ(render_texture.depth_stencil_format(), DepthStencilRenderBufferFormat::Default);
    ASSERT_EQ(render_texture.color_format(), ColorRenderBufferFormat::Default);
    ASSERT_EQ(render_texture.anti_aliasing_level(), AntiAliasingLevel{1});
}

TEST(RenderTexture, default_constructor_has_Tex2D_TextureDimensionality)
{
    const RenderTexture render_texture;
    ASSERT_EQ(render_texture.dimensionality(), TextureDimensionality::Tex2D);
}

TEST(RenderTexture, set_dimensionality_sets_the_dimensionality)
{
    RenderTexture render_texture;
    render_texture.set_dimensionality(TextureDimensionality::Cube);
    ASSERT_EQ(render_texture.dimensionality(), TextureDimensionality::Cube);
}

TEST(RenderTexture, set_dimensionality_to_Cube_throws_if_RenderTexture_is_multisampled)
{
    // edge-case: OpenGL doesn't support rendering to a multi-sampled cube texture,
    // so loudly throw an error if the caller is trying to render a multi-sampled
    // cubemap
    RenderTexture render_texture;
    render_texture.set_anti_aliasing_level(AntiAliasingLevel{2});
    ASSERT_ANY_THROW(render_texture.set_dimensionality(TextureDimensionality::Cube));
}

TEST(RenderTexture, set_anti_aliasing_level_throws_if_RenderRexture_dimensionality_is_Cube)
{
    // edge-case: OpenGL doesn't support rendering to a multi-sampled cube texture,
    // so loudly throw an error if the caller is trying to render a multi-sampled
    // cubemap
    RenderTexture render_texture;
    render_texture.set_dimensionality(TextureDimensionality::Cube);
    ASSERT_ANY_THROW(render_texture.set_anti_aliasing_level(AntiAliasingLevel{2}));
}

TEST(RenderTexture, constructor_throws_if_constructed_with_Cube_dimensionality_and_anti_aliasing)
{
    // edge-case: OpenGL doesn't support rendering to a multi-sampled cube texture,
    // so loudly throw an error if the caller is trying to render a multi-sampled
    // cubemap

    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    const RenderTextureParams render_texture_params = {
        .dimensionality = TextureDimensionality::Cube,
        .anti_aliasing_level = AntiAliasingLevel{2},
    };

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(const RenderTexture render_texture(render_texture_params));
}

TEST(RenderTexture, reformat_throws_if_given_CubeDimensionality_and_anti_aliasing)
{
    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    const RenderTextureParams render_texture_params = {
        .dimensionality = TextureDimensionality::Cube,
        .anti_aliasing_level = AntiAliasingLevel{2},
    };

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture().reformat(render_texture_params));
}

TEST(RenderTexture, throws_if_given_non_square_pixel_dimensions_but_Cube_dimensionality)
{
    // permitted
    const RenderTextureParams render_texture_params = {
        .pixel_dimensions = {1, 2},
        .dimensionality = TextureDimensionality::Cube,
    };

    // throws because non-square
    ASSERT_ANY_THROW(const RenderTexture render_texture(render_texture_params));
}

TEST(RenderTexture, set_dimensionality_throws_if_set_on_RenderTexture_with_non_square_pixel_dimensions)
{
    RenderTexture render_texture;
    render_texture.set_pixel_dimensions({1, 2});  // not square

    ASSERT_ANY_THROW(render_texture.set_dimensionality(TextureDimensionality::Cube));
}

TEST(RenderTexture, set_pixel_dimensions_throws_if_set_on_RenderTexture_with_cube_dimensionality)
{
    RenderTexture render_texture;
    render_texture.set_dimensionality(TextureDimensionality::Cube);

    ASSERT_ANY_THROW(render_texture.set_pixel_dimensions({1, 2}));
}

TEST(RenderTexture, set_dimension_changes_equality)
{
    const RenderTexture texture_a;
    RenderTexture texture_b{texture_a};

    ASSERT_EQ(texture_a, texture_b);

    texture_b.set_dimensionality(TextureDimensionality::Cube);

    ASSERT_NE(texture_a, texture_b);
}

TEST(RenderTexture, can_be_constructed_from_pixel_dimensions_vector)
{
    const Vector2i pixel_dimensions = {12, 12};
    const RenderTexture render_texture{{.pixel_dimensions = pixel_dimensions}};
    ASSERT_EQ(render_texture.pixel_dimensions(), pixel_dimensions);
}

TEST(RenderTexture, can_be_constructed_from_RenderTextureParams)
{
    const RenderTextureParams render_texture_parameters{{1, 1}};
    const RenderTexture render_texture{render_texture_parameters};
}

TEST(RenderTexture, FromDescriptorHasExpectedValues)
{
    const Vector2i pixel_dimensions = {8, 8};
    const AntiAliasingLevel aa_level{1};
    const ColorRenderBufferFormat format = ColorRenderBufferFormat::R8_UNORM;
    const TextureDimensionality dimensionality = TextureDimensionality::Cube;

    const RenderTextureParams render_texture_params = {
        .pixel_dimensions = pixel_dimensions,
        .dimensionality = dimensionality,
        .anti_aliasing_level = aa_level,
        .color_format = format,
    };

    const RenderTexture render_texture{render_texture_params};

    ASSERT_EQ(render_texture.pixel_dimensions(), pixel_dimensions);
    ASSERT_EQ(render_texture.dimensionality(), TextureDimensionality::Cube);
    ASSERT_EQ(render_texture.anti_aliasing_level(), aa_level);
    ASSERT_EQ(render_texture.color_format(), format);
}

TEST(RenderTexture, set_color_format_causes_color_to_return_set_value)
{
    const RenderTextureParams render_texture_params{{1, 1}};
    RenderTexture render_texture{render_texture_params};

    ASSERT_EQ(render_texture.color_format(), ColorRenderBufferFormat::Default);
    static_assert(ColorRenderBufferFormat::Default != ColorRenderBufferFormat::R8_UNORM);

    render_texture.set_color_format(ColorRenderBufferFormat::R8_UNORM);

    ASSERT_EQ(render_texture.color_format(), ColorRenderBufferFormat::R8_UNORM);
}

TEST(RenderTexture, upd_color_buffer_returns_independent_RenderBuffers_from_copies)
{
    // this popped up while developing the `LearnOpenGL/CSM` tab implementation, where
    // it was using a pattern like:
    //
    //     std::vector<RenderTexture> shadow_maps(num_cascades, RenderTexture{common_params});
    //
    // that pattern wasn't creating independent shadow maps because the underlying `RenderBuffer`s
    // were being reference-copied, rather than value-copied

    RenderTexture render_texture;
    RenderTexture render_texture_copy{render_texture};

    ASSERT_NE(render_texture_copy.upd_color_buffer(), render_texture.upd_color_buffer());
}

TEST(RenderTexture, upd_depth_buffer_returns_independent_RenderBuffers_from_copies)
{
    // this popped up while developing the `LearnOpenGL/CSM` tab implementation, where
    // it was using a pattern like:
    //
    //     std::vector<RenderTexture> shadow_maps(num_cascades, RenderTexture{common_params});
    //
    // that pattern wasn't creating independent shadow maps because the underlying `RenderBuffer`s
    // were being reference-copied, rather than value-copied

    RenderTexture rt;
    RenderTexture copy{rt};

    ASSERT_NE(copy.upd_depth_buffer(), rt.upd_depth_buffer());
}

TEST(RenderTexture, dimensions_equal_pixel_dimensions_on_construction)
{
    RenderTexture render_texture;
    render_texture.set_pixel_dimensions({7, 7});

    ASSERT_EQ(render_texture.pixel_dimensions(), Vector2i(7, 7));
    ASSERT_EQ(render_texture.dimensions(), Vector2(render_texture.pixel_dimensions()));
}

TEST(RenderTexture, dimensions_are_scaled_by_device_pixel_ratio)
{
    RenderTexture render_texture;
    render_texture.set_pixel_dimensions({7, 7});

    ASSERT_EQ(render_texture.dimensions(), Vector2(7.0f, 7.0f));
    render_texture.set_device_pixel_ratio(2.0f);
    ASSERT_EQ(render_texture.dimensions(), Vector2(7.0f,7.0f)/2.0f);
    render_texture.set_device_pixel_ratio(0.5f);
    ASSERT_EQ(render_texture.dimensions(), Vector2(7.0f,7.0f)/0.5f);
}

TEST(RenderTexture, device_pixel_ratio_is_initially_1)
{
    const RenderTexture render_texture;
    ASSERT_EQ(render_texture.device_pixel_ratio(), 1.0f);
}

TEST(RenderTexture, set_device_pixel_ratio_sets_pixel_ratio)
{
    RenderTexture render_texture;
    ASSERT_EQ(render_texture.device_pixel_ratio(), 1.0f);
    render_texture.set_device_pixel_ratio(2.0f);
    ASSERT_EQ(render_texture.device_pixel_ratio(), 2.0f);
    render_texture.set_device_pixel_ratio(0.25f);
    ASSERT_EQ(render_texture.device_pixel_ratio(), 0.25f);
}

TEST(RenderTexture, device_pixel_ratio_is_propagated_from_params)
{
    const RenderTexture render_texture{{
        .device_pixel_ratio = 3.0f,
    }};
    ASSERT_EQ(render_texture.device_pixel_ratio(), 3.0f);
}

TEST(RenderTexture, device_pixel_ratio_from_params_affects_dimensions)
{
    const RenderTexture render_texture{{
        .pixel_dimensions = {13, 13},
        .device_pixel_ratio = 2.5f,
    }};
    ASSERT_EQ(render_texture.pixel_dimensions(), Vector2i(13, 13));
    ASSERT_EQ(render_texture.device_pixel_ratio(), 2.5f);
    ASSERT_EQ(render_texture.dimensions(), Vector2(13.0f, 13.0f)/2.5f);
}

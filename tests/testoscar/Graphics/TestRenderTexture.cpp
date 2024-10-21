#include <oscar/Graphics/RenderTexture.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/ColorRenderBufferFormat.h>
#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <oscar/Graphics/RenderTextureParams.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>

using namespace osc;

TEST(RenderTexture, default_constructor_creates_1x1_default_texture)
{
    const RenderTexture render_texture;
    ASSERT_EQ(render_texture.dimensions(), Vec2i(1, 1));
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
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap
    RenderTexture render_texture;
    render_texture.set_anti_aliasing_level(AntiAliasingLevel{2});
    ASSERT_ANY_THROW(render_texture.set_dimensionality(TextureDimensionality::Cube));
}

TEST(RenderTexture, set_anti_aliasing_level_throws_if_RenderRexture_dimensionality_is_Cube)
{
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap
    RenderTexture render_texture;
    render_texture.set_dimensionality(TextureDimensionality::Cube);
    ASSERT_ANY_THROW(render_texture.set_anti_aliasing_level(AntiAliasingLevel{2}));
}

TEST(RenderTexture, constructor_throws_if_constructed_with_Cube_dimensionality_and_anti_aliasing)
{
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap

    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    const RenderTextureParams render_texture_params = {
        .dimensionality = TextureDimensionality::Cube,
        .anti_aliasing_level = AntiAliasingLevel{2},
    };

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture render_texture(render_texture_params));
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

TEST(RenderTexture, throws_if_given_non_square_dimensions_but_Cube_dimensionality)
{
    // permitted
    const RenderTextureParams render_texture_params = {
        .dimensions = {1, 2},
        .dimensionality = TextureDimensionality::Cube,
    };

    // throws because non-square
    ASSERT_ANY_THROW(RenderTexture render_texture(render_texture_params));
}

TEST(RenderTexture, set_dimensionality_throws_if_set_on_RenderTexture_with_non_square_dimensions)
{
    RenderTexture render_texture;
    render_texture.set_dimensions({1, 2});  // not square

    ASSERT_ANY_THROW(render_texture.set_dimensionality(TextureDimensionality::Cube));
}

TEST(RenderTexture, set_dimensions_throws_if_set_on_RenderTexture_with_cube_dimensionality)
{
    RenderTexture render_texture;
    render_texture.set_dimensionality(TextureDimensionality::Cube);

    ASSERT_ANY_THROW(render_texture.set_dimensions({1, 2}));
}

TEST(RenderTexture, set_dimension_changes_equality)
{
    RenderTexture texture_a;
    RenderTexture texture_b{texture_a};

    ASSERT_EQ(texture_a, texture_b);

    texture_b.set_dimensionality(TextureDimensionality::Cube);

    ASSERT_NE(texture_a, texture_b);
}

TEST(RenderTexture, can_be_constructed_from_dimensions_vector)
{
    const Vec2i dimensions = {12, 12};
    RenderTexture render_texture{{.dimensions = dimensions}};
    ASSERT_EQ(render_texture.dimensions(), dimensions);
}

TEST(RenderTexture, can_be_constructed_from_RenderTextureParams)
{
    const RenderTextureParams render_texture_parameters{{1, 1}};
    const RenderTexture render_texture{render_texture_parameters};
}

TEST(RenderTexture, FromDescriptorHasExpectedValues)
{
    const Vec2i dimensions = {8, 8};
    const AntiAliasingLevel aa_level{1};
    const ColorRenderBufferFormat format = ColorRenderBufferFormat::R8_UNORM;
    const TextureDimensionality dimensionality = TextureDimensionality::Cube;

    const RenderTextureParams render_texture_params = {
        .dimensions = dimensions,
        .dimensionality = dimensionality,
        .anti_aliasing_level = aa_level,
        .color_format = format,
    };

    const RenderTexture render_texture{render_texture_params};

    ASSERT_EQ(render_texture.dimensions(), dimensions);
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
    //     std::vector<RenderTexture> shadowmaps(num_cascades, RenderTexture{common_params});
    //
    // that pattern wasn't creating independent shadowmaps because the underlying `RenderBuffer`s
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
    //     std::vector<RenderTexture> shadowmaps(num_cascades, RenderTexture{common_params});
    //
    // that pattern wasn't creating independent shadowmaps because the underlying `RenderBuffer`s
    // were being reference-copied, rather than value-copied

    RenderTexture rt;
    RenderTexture copy{rt};

    ASSERT_NE(copy.upd_depth_buffer(), rt.upd_depth_buffer());
}

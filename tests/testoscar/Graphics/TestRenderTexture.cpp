#include <oscar/Graphics/RenderTexture.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureParams.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>

using namespace osc;

TEST(RenderTexture, DefaultConstructorCreates1x1DefaultRenderTexture)
{
    const RenderTexture tex;
    ASSERT_EQ(tex.dimensions(), Vec2i(1, 1));
    ASSERT_EQ(tex.depth_stencil_format(), DepthStencilFormat::Default);
    ASSERT_EQ(tex.color_format(), RenderTextureFormat::ARGB32);
    ASSERT_EQ(tex.anti_aliasing_level(), AntiAliasingLevel{1});
}

TEST(RenderTexture, DefaultConstructorHasTex2DDimension)
{
    const RenderTexture tex;
    ASSERT_EQ(tex.dimensionality(), TextureDimensionality::Tex2D);
}

TEST(RenderTexture, SetDimensionSetsTheDimension)
{
    RenderTexture tex;
    tex.set_dimensionality(TextureDimensionality::Cube);
    ASSERT_EQ(tex.dimensionality(), TextureDimensionality::Cube);
}

TEST(RenderTexture, SetDimensionToCubeThrowsIfRenderTextureIsMultisampled)
{
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap
    RenderTexture tex;
    tex.set_anti_aliasing_level(AntiAliasingLevel{2});
    ASSERT_ANY_THROW(tex.set_dimensionality(TextureDimensionality::Cube));
}

TEST(RenderTexture, SetAntialiasingToNonOneOnCubeDimensionalityRenderTextureThrows)
{
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap
    RenderTexture tex;
    tex.set_dimensionality(TextureDimensionality::Cube);
    ASSERT_ANY_THROW(tex.set_anti_aliasing_level(AntiAliasingLevel{2}));
}

TEST(RenderTexture, CtorThrowsIfGivenCubeDimensionalityAndAntialiasedDescriptor)
{
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap

    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    const RenderTextureParams desc = {
        .dimensionality = TextureDimensionality::Cube,
        .anti_aliasing_level = AntiAliasingLevel{2},
    };

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture rt(desc));
}

TEST(RenderTexture, ReformatThrowsIfGivenCubeDimensionalityAndAntialiasedDescriptor)
{
    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    const RenderTextureParams params = {
        .dimensionality = TextureDimensionality::Cube,
        .anti_aliasing_level = AntiAliasingLevel{2},
    };

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture().reformat(params));
}

TEST(RenderTexture, ThrowsIfGivenNonSquareButCubeDimensionalityDescriptor)
{
    // permitted
    const RenderTextureParams params = {
        .dimensions = {1, 2},
        .dimensionality = TextureDimensionality::Cube,
    };

    // throws because non-square
    ASSERT_ANY_THROW(RenderTexture rt(params));
}

TEST(RenderTexture, ReformatThrowsIfGivenNonSquareButCubeDimensionalityDescriptor)
{
    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    const RenderTextureParams params = {
        .dimensions = {1, 2},
        .dimensionality = TextureDimensionality::Cube,
    };

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture().reformat(params));
}

TEST(RenderTexture, SetDimensionThrowsIfSetToCubeOnNonSquareRenderTexture)
{
    RenderTexture t;
    t.set_dimensions({1, 2});  // not square

    ASSERT_ANY_THROW(t.set_dimensionality(TextureDimensionality::Cube));
}

TEST(RenderTexture, SetDimensionsThrowsIfSettingNonSquareOnCubeDimensionTexture)
{
    RenderTexture t;
    t.set_dimensionality(TextureDimensionality::Cube);

    ASSERT_ANY_THROW(t.set_dimensions({1, 2}));
}

TEST(RenderTexture, SetDimensionChangesEquality)
{
    RenderTexture t1;
    RenderTexture t2{t1};

    ASSERT_EQ(t1, t2);

    t2.set_dimensionality(TextureDimensionality::Cube);

    ASSERT_NE(t1, t2);
}

TEST(RenderTexture, CanBeConstructedFromDimensions)
{
    const Vec2i dims = {12, 12};
    RenderTexture tex{{.dimensions = dims}};
    ASSERT_EQ(tex.dimensions(), dims);
}

TEST(RenderTexture, CanBeConstructedFromADescriptor)
{
    const RenderTextureParams params{{1, 1}};
    const RenderTexture d{params};
}

TEST(RenderTexture, DefaultCtorAssignsDefaultReadWrite)
{
    RenderTexture t;

    ASSERT_EQ(t.read_write(), RenderTextureReadWrite::Default);
}

TEST(RenderTexture, FromDescriptorHasExpectedValues)
{
    const int width = 8;
    const int height = 8;
    const AntiAliasingLevel aaLevel{1};
    const RenderTextureFormat format = RenderTextureFormat::Red8;
    const RenderTextureReadWrite rw = RenderTextureReadWrite::Linear;
    const TextureDimensionality dimension = TextureDimensionality::Cube;

    const RenderTextureParams params = {
        .dimensions = {width, height},
        .dimensionality = dimension,
        .anti_aliasing_level = aaLevel,
        .color_format = format,
        .read_write = rw,
    };

    const RenderTexture tex{params};

    ASSERT_EQ(tex.dimensions(), Vec2i(width, height));
    ASSERT_EQ(tex.dimensionality(), TextureDimensionality::Cube);
    ASSERT_EQ(tex.anti_aliasing_level(), aaLevel);
    ASSERT_EQ(tex.color_format(), format);
    ASSERT_EQ(tex.read_write(), rw);
}

TEST(RenderTexture, SetColorFormatCausesGetColorFormatToReturnValue)
{
    const RenderTextureParams params{{1, 1}};
    RenderTexture d{params};

    ASSERT_EQ(d.color_format(), RenderTextureFormat::ARGB32);

    d.set_color_format(RenderTextureFormat::Red8);

    ASSERT_EQ(d.color_format(), RenderTextureFormat::Red8);
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

    RenderTexture rt;
    RenderTexture copy{rt};

    ASSERT_NE(copy.upd_color_buffer(), rt.upd_color_buffer());
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

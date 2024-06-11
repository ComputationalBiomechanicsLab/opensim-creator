#include <oscar/Graphics/RenderTexture.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/RenderTextureDescriptor.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>

using namespace osc;

TEST(RenderTexture, DefaultConstructorCreates1x1RgbaRenderTexture)
{
    const RenderTexture tex;
    ASSERT_EQ(tex.dimensions(), Vec2i(1, 1));
    ASSERT_EQ(tex.depth_stencil_format(), DepthStencilFormat::D24_UNorm_S8_UInt);
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
    RenderTextureDescriptor desc{{1, 1}};

    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    desc.set_anti_aliasing_level(AntiAliasingLevel{2});
    desc.set_dimensionality(TextureDimensionality::Cube);

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture rt(desc));
}

TEST(RenderTexture, ReformatThrowsIfGivenCubeDimensionalityAndAntialiasedDescriptor)
{
    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    RenderTextureDescriptor desc{{1, 1}};
    desc.set_anti_aliasing_level(AntiAliasingLevel{2});
    desc.set_dimensionality(TextureDimensionality::Cube);

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture().reformat(desc));
}

TEST(RenderTexture, ThrowsIfGivenNonSquareButCubeDimensionalityDescriptor)
{
    RenderTextureDescriptor desc{{1, 2}};  // not square
    desc.set_dimensionality(TextureDimensionality::Cube);  // permitted, at least for now

    ASSERT_ANY_THROW(RenderTexture rt(desc));
}

TEST(RenderTexture, ReformatThrowsIfGivenNonSquareButCubeDimensionalityDescriptor)
{
    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    RenderTextureDescriptor desc{{1, 2}};
    desc.set_dimensionality(TextureDimensionality::Cube);

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture().reformat(desc));
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
    RenderTexture tex{dims};
    ASSERT_EQ(tex.dimensions(), dims);
}

TEST(RenderTexture, CanBeConstructedFromADescriptor)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTexture d{d1};
}

TEST(RenderTexture, DefaultCtorAssignsDefaultReadWrite)
{
    RenderTexture t;

    ASSERT_EQ(t.read_write(), RenderTextureReadWrite::Default);
}

TEST(RenderTexture, FromDescriptorHasExpectedValues)
{
    int width = 8;
    int height = 8;
    AntiAliasingLevel aaLevel{1};
    RenderTextureFormat format = RenderTextureFormat::Red8;
    RenderTextureReadWrite rw = RenderTextureReadWrite::Linear;
    TextureDimensionality dimension = TextureDimensionality::Cube;

    RenderTextureDescriptor desc{{width, height}};
    desc.set_dimensionality(dimension);
    desc.set_anti_aliasing_level(aaLevel);
    desc.set_color_format(format);
    desc.set_read_write(rw);

    RenderTexture tex{desc};

    ASSERT_EQ(tex.dimensions(), Vec2i(width, height));
    ASSERT_EQ(tex.dimensionality(), TextureDimensionality::Cube);
    ASSERT_EQ(tex.anti_aliasing_level(), aaLevel);
    ASSERT_EQ(tex.color_format(), format);
    ASSERT_EQ(tex.read_write(), rw);
}

TEST(RenderTexture, SetColorFormatCausesGetColorFormatToReturnValue)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTexture d{d1};

    ASSERT_EQ(d.color_format(), RenderTextureFormat::ARGB32);

    d.set_color_format(RenderTextureFormat::Red8);

    ASSERT_EQ(d.color_format(), RenderTextureFormat::Red8);
}

TEST(RenderTexture, UpdColorBufferReturnsNonNullPtr)
{
    RenderTexture rt{{1, 1}};

    ASSERT_NE(rt.upd_color_buffer(), nullptr);
}

TEST(RenderTexture, UpdDepthBufferReturnsNonNullPtr)
{
    RenderTexture rt{{1, 1}};

    ASSERT_NE(rt.upd_depth_buffer(), nullptr);
}

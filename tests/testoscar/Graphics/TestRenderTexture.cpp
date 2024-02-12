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
    RenderTexture const tex;
    ASSERT_EQ(tex.getDimensions(), Vec2i(1, 1));
    ASSERT_EQ(tex.getDepthStencilFormat(), DepthStencilFormat::D24_UNorm_S8_UInt);
    ASSERT_EQ(tex.getColorFormat(), RenderTextureFormat::ARGB32);
    ASSERT_EQ(tex.getAntialiasingLevel(), AntiAliasingLevel{1});
}

TEST(RenderTexture, DefaultConstructorHasTex2DDimension)
{
    RenderTexture const tex;
    ASSERT_EQ(tex.getDimensionality(), TextureDimensionality::Tex2D);
}

TEST(RenderTexture, SetDimensionSetsTheDimension)
{
    RenderTexture tex;
    tex.setDimensionality(TextureDimensionality::Cube);
    ASSERT_EQ(tex.getDimensionality(), TextureDimensionality::Cube);
}

TEST(RenderTexture, SetDimensionToCubeThrowsIfRenderTextureIsMultisampled)
{
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap
    RenderTexture tex;
    tex.setAntialiasingLevel(AntiAliasingLevel{2});
    ASSERT_ANY_THROW(tex.setDimensionality(TextureDimensionality::Cube));
}

TEST(RenderTexture, SetAntialiasingToNonOneOnCubeDimensionalityRenderTextureThrows)
{
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap
    RenderTexture tex;
    tex.setDimensionality(TextureDimensionality::Cube);
    ASSERT_ANY_THROW(tex.setAntialiasingLevel(AntiAliasingLevel{2}));
}

TEST(RenderTexture, CtorThrowsIfGivenCubeDimensionalityAndAntialiasedDescriptor)
{
    // edge-case: OpenGL doesn't support rendering to a multisampled cube texture,
    // so loudly throw an error if the caller is trying to render a multisampled
    // cubemap
    RenderTextureDescriptor desc{{1, 1}};

    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    desc.setAntialiasingLevel(AntiAliasingLevel{2});
    desc.setDimensionality(TextureDimensionality::Cube);

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture rt(desc));
}

TEST(RenderTexture, ReformatThrowsIfGivenCubeDimensionalityAndAntialiasedDescriptor)
{
    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    RenderTextureDescriptor desc{{1, 1}};
    desc.setAntialiasingLevel(AntiAliasingLevel{2});
    desc.setDimensionality(TextureDimensionality::Cube);

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture().reformat(desc));
}

TEST(RenderTexture, ThrowsIfGivenNonSquareButCubeDimensionalityDescriptor)
{
    RenderTextureDescriptor desc{{1, 2}};  // not square
    desc.setDimensionality(TextureDimensionality::Cube);  // permitted, at least for now

    ASSERT_ANY_THROW(RenderTexture rt(desc));
}

TEST(RenderTexture, ReformatThrowsIfGivenNonSquareButCubeDimensionalityDescriptor)
{
    // allowed: RenderTextureDescriptor is non-throwing until the texture is actually constructed
    RenderTextureDescriptor desc{{1, 2}};
    desc.setDimensionality(TextureDimensionality::Cube);

    // throws because the descriptor is bad
    ASSERT_ANY_THROW(RenderTexture().reformat(desc));
}

TEST(RenderTexture, SetDimensionThrowsIfSetToCubeOnNonSquareRenderTexture)
{
    RenderTexture t;
    t.setDimensions({1, 2});  // not square

    ASSERT_ANY_THROW(t.setDimensionality(TextureDimensionality::Cube));
}

TEST(RenderTexture, SetDimensionsThrowsIfSettingNonSquareOnCubeDimensionTexture)
{
    RenderTexture t;
    t.setDimensionality(TextureDimensionality::Cube);

    ASSERT_ANY_THROW(t.setDimensions({1, 2}));
}

TEST(RenderTexture, SetDimensionChangesEquality)
{
    RenderTexture t1;
    RenderTexture t2{t1};

    ASSERT_EQ(t1, t2);

    t2.setDimensionality(TextureDimensionality::Cube);

    ASSERT_NE(t1, t2);
}

TEST(RenderTexture, CanBeConstructedFromDimensions)
{
    Vec2i const dims = {12, 12};
    RenderTexture tex{dims};
    ASSERT_EQ(tex.getDimensions(), dims);
}

TEST(RenderTexture, CanBeConstructedFromADescriptor)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTexture d{d1};
}

TEST(RenderTexture, DefaultCtorAssignsDefaultReadWrite)
{
    RenderTexture t;

    ASSERT_EQ(t.getReadWrite(), RenderTextureReadWrite::Default);
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
    desc.setDimensionality(dimension);
    desc.setAntialiasingLevel(aaLevel);
    desc.setColorFormat(format);
    desc.setReadWrite(rw);

    RenderTexture tex{desc};

    ASSERT_EQ(tex.getDimensions(), Vec2i(width, height));
    ASSERT_EQ(tex.getDimensionality(), TextureDimensionality::Cube);
    ASSERT_EQ(tex.getAntialiasingLevel(), aaLevel);
    ASSERT_EQ(tex.getColorFormat(), format);
    ASSERT_EQ(tex.getReadWrite(), rw);
}

TEST(RenderTexture, SetColorFormatCausesGetColorFormatToReturnValue)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTexture d{d1};

    ASSERT_EQ(d.getColorFormat(), RenderTextureFormat::ARGB32);

    d.setColorFormat(RenderTextureFormat::Red8);

    ASSERT_EQ(d.getColorFormat(), RenderTextureFormat::Red8);
}

TEST(RenderTexture, UpdColorBufferReturnsNonNullPtr)
{
    RenderTexture rt{{1, 1}};

    ASSERT_NE(rt.updColorBuffer(), nullptr);
}

TEST(RenderTexture, UpdDepthBufferReturnsNonNullPtr)
{
    RenderTexture rt{{1, 1}};

    ASSERT_NE(rt.updDepthBuffer(), nullptr);
}

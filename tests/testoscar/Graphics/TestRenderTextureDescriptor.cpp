#include <oscar/Graphics/RenderTextureDescriptor.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/StringHelpers.h>

using osc::AntiAliasingLevel;
using osc::ContainsCaseInsensitive;
using osc::DepthStencilFormat;
using osc::RenderTextureDescriptor;
using osc::RenderTextureFormat;
using osc::RenderTextureReadWrite;
using osc::TextureDimensionality;
using osc::Vec2i;

TEST(RenderTextureDescriptor, CanBeConstructedFromWithAndHeight)
{
    RenderTextureDescriptor d{{1, 1}};
}

TEST(RenderTextureDescriptor, CoercesNegativeWidthsToZero)
{
    RenderTextureDescriptor d{{-1, 1}};

    ASSERT_EQ(d.getDimensions().x, 0);
}

TEST(RenderTextureDescriptor, CoercesNegativeHeightsToZero)
{
    RenderTextureDescriptor d{{1, -1}};

    ASSERT_EQ(d.getDimensions().y, 0);
}

TEST(RenderTextureDescriptor, CanBeCopyConstructed)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor{d1};
}

TEST(RenderTextureDescriptor, CanBeCopyAssigned)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{{1, 1}};
    d1 = d2;
}

TEST(RenderTextureDescriptor, GetWidthReturnsConstructedWith)
{
    int width = 1;
    RenderTextureDescriptor d1{{width, 1}};
    ASSERT_EQ(d1.getDimensions().x, width);
}

TEST(RenderTextureDescriptor, SetWithFollowedByGetWithReturnsSetWidth)
{
    RenderTextureDescriptor d1{{1, 1}};


    int newWidth = 31;
    Vec2i d = d1.getDimensions();
    d.x = newWidth;

    d1.setDimensions(d);
    ASSERT_EQ(d1.getDimensions(), d);
}

TEST(RenderTextureDescriptor, SetWidthNegativeValueThrows)
{
    RenderTextureDescriptor d1{{1, 1}};
    ASSERT_ANY_THROW({ d1.setDimensions({-1, 1}); });
}

TEST(RenderTextureDescriptor, GetHeightReturnsConstructedHeight)
{
    int height = 1;
    RenderTextureDescriptor d1{{1, height}};
    ASSERT_EQ(d1.getDimensions().y, height);
}

TEST(RenderTextureDescriptor, SetHeightFollowedByGetHeightReturnsSetHeight)
{
    RenderTextureDescriptor d1{{1, 1}};

    Vec2i d = d1.getDimensions();
    d.y = 31;

    d1.setDimensions(d);

    ASSERT_EQ(d1.getDimensions(), d);
}

TEST(RenderTextureDescriptor, GetAntialiasingLevelInitiallyReturns1)
{
    RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getAntialiasingLevel(), AntiAliasingLevel{1});
}

TEST(RenderTextureDescriptor, SetAntialiasingLevelMakesGetAntialiasingLevelReturnValue)
{
    AntiAliasingLevel newAntialiasingLevel{4};

    RenderTextureDescriptor d1{{1, 1}};
    d1.setAntialiasingLevel(newAntialiasingLevel);
    ASSERT_EQ(d1.getAntialiasingLevel(), newAntialiasingLevel);
}

TEST(RenderTextureDescriptor, GetColorFormatReturnsARGB32ByDefault)
{
    RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getColorFormat(), RenderTextureFormat::ARGB32);
}

TEST(RenderTextureDescriptor, SetColorFormatMakesGetColorFormatReturnTheFormat)
{
    RenderTextureDescriptor d{{1, 1}};

    ASSERT_EQ(d.getColorFormat(), RenderTextureFormat::ARGB32);

    d.setColorFormat(RenderTextureFormat::Red8);

    ASSERT_EQ(d.getColorFormat(), RenderTextureFormat::Red8);
}

TEST(RenderTextureDescriptor, GetDepthStencilFormatReturnsDefaultValue)
{
    RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getDepthStencilFormat(), DepthStencilFormat::D24_UNorm_S8_UInt);
}

TEST(RenderTextureDescriptor, StandardCtorGetReadWriteReturnsDefaultValue)
{
    RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getReadWrite(), RenderTextureReadWrite::Default);
}

TEST(RenderTextureDescriptor, SetReadWriteMakesGetReadWriteReturnNewValue)
{
    RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getReadWrite(), RenderTextureReadWrite::Default);

    d1.setReadWrite(RenderTextureReadWrite::Linear);

    ASSERT_EQ(d1.getReadWrite(), RenderTextureReadWrite::Linear);
}

TEST(RenderTextureDescriptor, GetDimensionReturns2DOnConstruction)
{
    RenderTextureDescriptor d1{{1, 1}};

    ASSERT_EQ(d1.getDimensionality(), TextureDimensionality::Tex2D);
}

TEST(RenderTextureDescriptor, SetDimensionCausesGetDimensionToReturnTheSetDimension)
{
    RenderTextureDescriptor d1{{1, 1}};
    d1.setDimensionality(TextureDimensionality::Cube);

    ASSERT_EQ(d1.getDimensionality(), TextureDimensionality::Cube);
}

TEST(RenderTextureDescriptor, SetDimensionChangesDescriptorEquality)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{d1};

    ASSERT_EQ(d1, d2);

    d1.setDimensionality(TextureDimensionality::Cube);

    ASSERT_NE(d1, d2);
}

TEST(RenderTextureDescriptor, SetDimensionToCubeOnRectangularDimensionsCausesNoError)
{
    // logically, a cubemap's dimensions must be square, but RenderTextureDescriptor
    // allows changing the dimension independently from changing the dimensions without
    // throwing an error, so that code like:
    //
    // desc.setDimensionality(TextureDimensionality::Cube);
    // desc.setDimensions({2,2});
    //
    // is permitted, even though the first line might create an "invalid" descriptor

    RenderTextureDescriptor rect{{1, 2}};
    rect.setDimensionality(TextureDimensionality::Cube);

    // also permitted
    RenderTextureDescriptor initiallySquare{{1, 1}};
    initiallySquare.setDimensions({1, 2});
    initiallySquare.setDimensionality(TextureDimensionality::Cube);
}

TEST(RenderTextureDescriptor, SetReadWriteChangesEquality)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{d1};

    ASSERT_EQ(d1, d2);

    d2.setReadWrite(RenderTextureReadWrite::Linear);

    ASSERT_NE(d1, d2);
}

TEST(RenderTextureDescriptor, ComparesEqualOnCopyConstruct)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{d1};

    ASSERT_EQ(d1, d2);
}

TEST(RenderTextureDescriptor, ComparesEqualWithSameConstructionVals)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{{1, 1}};

    ASSERT_EQ(d1, d2);
}

TEST(RenderTextureDescriptor, SetDimensionsWidthMakesItCompareNotEqual)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{{1, 1}};

    d2.setDimensions({2, 1});

    ASSERT_NE(d1, d2);
}

TEST(RenderTextureDescriptor, SetDimensionsHeightMakesItCompareNotEqual)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{{1, 1}};

    d2.setDimensions({1, 2});

    ASSERT_NE(d1, d2);
}

TEST(RenderTextureDescriptor, SetAntialiasingLevelMakesItCompareNotEqual)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{{1, 1}};

    d2.setAntialiasingLevel(AntiAliasingLevel{2});

    ASSERT_NE(d1, d2);
}

TEST(RenderTextureDescriptor, SetAntialiasingLevelToSameValueComparesEqual)
{
    RenderTextureDescriptor d1{{1, 1}};
    RenderTextureDescriptor d2{{1, 1}};

    d2.setAntialiasingLevel(d2.getAntialiasingLevel());

    ASSERT_EQ(d1, d2);
}

TEST(RenderTextureDescriptor, CanBeStreamedToAString)
{
    RenderTextureDescriptor d1{{1, 1}};
    std::stringstream ss;
    ss << d1;

    std::string str{ss.str()};
    ASSERT_TRUE(ContainsCaseInsensitive(str, "RenderTextureDescriptor"));
}

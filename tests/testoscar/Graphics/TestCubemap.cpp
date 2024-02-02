#include <oscar/Graphics/Cubemap.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

using osc::Cubemap;
using osc::CubemapFace;
using osc::TextureFilterMode;
using osc::TextureFormat;
using osc::TextureWrapMode;

TEST(Cubemap, CanConstruct1x1RGBA32Cubemap)
{
    Cubemap cubemap{1, TextureFormat::RGBA32};
}

TEST(Cubemap, ConstructorThrowsIfGivenZeroWidth)
{
    ASSERT_ANY_THROW({ Cubemap cubemap(0, TextureFormat::RGBA32); });
}

TEST(Cubemap, ConstructorThrowsIfGivenNegativeWidth)
{
    ASSERT_ANY_THROW({ Cubemap cubemap(-5, TextureFormat::RGBA32); });
}

TEST(Cubemap, CanBeCopyConstructed)
{
    Cubemap const source{1, TextureFormat::RGBA32};
    Cubemap{source};
}

static_assert(std::is_nothrow_move_constructible_v<Cubemap>);

TEST(Cubemap, CanBeMoveConstructed)
{
    Cubemap source{1, TextureFormat::RGBA32};
    Cubemap{std::move(source)};
}

TEST(Cubemap, CanBeCopyAssigned)
{
    Cubemap const first{1, TextureFormat::RGBA32};
    Cubemap second{2, TextureFormat::RGB24};
    second = first;

    ASSERT_EQ(second.getWidth(), first.getWidth());
    ASSERT_EQ(second.getTextureFormat(), first.getTextureFormat());
}

static_assert(std::is_nothrow_assignable_v<Cubemap, Cubemap&&>);

TEST(Cubemap, CanBeMoveAssigned)
{
    int32_t const firstWidth = 1;
    TextureFormat const firstFormat = TextureFormat::RGB24;
    Cubemap first{firstWidth, firstFormat};

    int32_t const secondWidth = 2;
    TextureFormat const secondFormat = TextureFormat::RGBA32;
    Cubemap second{secondWidth, secondFormat};
    second = std::move(first);

    ASSERT_NE(firstWidth, secondWidth);
    ASSERT_NE(firstFormat, secondFormat);
    ASSERT_EQ(second.getWidth(), firstWidth);
    ASSERT_EQ(second.getTextureFormat(), firstFormat);
}

static_assert(std::is_nothrow_destructible_v<Cubemap>);

TEST(Cubemap, CanBeReferenceComparedForEquality)
{
    Cubemap const cubemap{1, TextureFormat::RGBA32};

    ASSERT_EQ(cubemap, cubemap);
}

TEST(Cubemap, CopiesCompareEqual)
{
    Cubemap const cubemap{1, TextureFormat::RGBA32};
    Cubemap const copy{cubemap}; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(cubemap, copy);
}

TEST(Cubemap, MutatingACopyMakesItNotEqual)
{
    Cubemap const cubemap{1, TextureFormat::RGBA32};

    Cubemap copy{cubemap};
    std::array<uint8_t, 4> const data = {};
    copy.setPixelData(CubemapFace::PositiveX, data);

    ASSERT_NE(cubemap, copy);
}

TEST(Cubemap, EqualityIsReferenceAndNotValueBased)
{
    // landmine test: this just verifies that equality is
    // really just reference equality, rather than actual
    // value equality (which is better)
    //
    // if the implementation of Cubemap has been updated
    // to enabled value-equality (e.g. by comparing the actual
    // image data or using a strong hashing technique) then
    // this test can be deleted
    Cubemap const a{1, TextureFormat::RGBA32};
    Cubemap const b{1, TextureFormat::RGBA32};

    ASSERT_NE(a, b);
}

TEST(Cubemap, GetWidthReturnsConstructedWidth)
{
    int32_t const width = 4;
    Cubemap const cubemap{width, TextureFormat::RGBA32};

    ASSERT_EQ(cubemap.getWidth(), width);
}

TEST(Cubemap, GetFormatReturnsConstructedFormat)
{
    TextureFormat const format = TextureFormat::RGB24;
    Cubemap const cubemap{1, format};

    ASSERT_EQ(cubemap.getTextureFormat(), format);
}

TEST(Cubemap, SetDataWorksForAnyFaceIfGivenCorrectNumberOfBytes)
{
    TextureFormat const format = TextureFormat::RGBA32;
    constexpr size_t bytesPerPixelForFormat = 4;
    constexpr size_t width = 5;
    constexpr size_t nPixels = width*width*bytesPerPixelForFormat;
    std::array<uint8_t, nPixels> const data = {};

    Cubemap cubemap{width, format};
    for (CubemapFace face = osc::FirstCubemapFace(); face <= osc::LastCubemapFace(); face = osc::Next(face))
    {
        cubemap.setPixelData(face, data);
    }
}

TEST(Cubemap, SetDataThrowsIfGivenIncorrectNumberOfBytesForRGBA32)
{
    TextureFormat const format = TextureFormat::RGBA32;
    constexpr size_t incorrectBytesPerPixelForFormat = 3;
    constexpr size_t width = 5;
    constexpr size_t nPixels = width*width*incorrectBytesPerPixelForFormat;
    std::array<uint8_t, nPixels> const data = {};

    Cubemap cubemap{width, format};
    for (CubemapFace face = osc::FirstCubemapFace(); face <= osc::LastCubemapFace(); face = osc::Next(face))
    {
        ASSERT_ANY_THROW({ cubemap.setPixelData(face, data); });
    }
}

TEST(Cubemap, SetDataThrowsIfGivenIncorrectNumberOfBytesForRGB24)
{
    TextureFormat const format = TextureFormat::RGB24;
    constexpr size_t incorrectBytesPerPixelForFormat = 4;
    constexpr size_t width = 5;
    constexpr size_t nPixels = width*width*incorrectBytesPerPixelForFormat;
    std::array<uint8_t, nPixels> const data = {};

    Cubemap cubemap{width, format};
    for (CubemapFace face = osc::FirstCubemapFace(); face <= osc::LastCubemapFace(); face = osc::Next(face))
    {
        ASSERT_ANY_THROW({ cubemap.setPixelData(face, data); });
    }
}

TEST(Cubemap, SetDataThrowsIfGivenIncorrectNumberOfBytesForWidth)
{
    TextureFormat const format = TextureFormat::RGBA32;
    constexpr size_t bytesPerPixelForFormat = 4;
    constexpr size_t width = 5;
    constexpr size_t nPixels = width*width*bytesPerPixelForFormat;
    constexpr size_t incorrectNPixels = nPixels+3;
    std::array<uint8_t, incorrectNPixels> const data = {};

    Cubemap cubemap{width, format};
    for (CubemapFace face = osc::FirstCubemapFace(); face <= osc::LastCubemapFace(); face = osc::Next(face))
    {
        ASSERT_ANY_THROW({ cubemap.setPixelData(face, data); });
    }
}

TEST(Cubemap, SetPixelDataWorksWithFloatingPointTextureFormats)
{
    [[maybe_unused]] TextureFormat const format = TextureFormat::RGBAFloat;
}

TEST(Cubemap, GetWrapModeInitiallyReturnsRepeatedByDefault)
{
    ASSERT_EQ(Cubemap(1, TextureFormat::RGBA32).getWrapMode(), TextureWrapMode::Repeat);
}

TEST(Cubemap, SetWrapModeUpdatesWrapMode)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Repeat);
    cm.setWrapMode(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Clamp);
}

TEST(Cubemap, SetWrapModeSetsAllAxes)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeU(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeV(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeW(), TextureWrapMode::Repeat);
    cm.setWrapMode(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapModeU(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapModeV(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapModeW(), TextureWrapMode::Clamp);
}

TEST(Cubemap, SetWrapModeUSetsUAxisAndGetWrapModeRv)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeU(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeV(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeW(), TextureWrapMode::Repeat);
    cm.setWrapModeU(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapModeU(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapModeV(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeW(), TextureWrapMode::Repeat);
}

TEST(Cubemap, SetWrapModeVOnlySetsVAxis)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeU(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeV(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeW(), TextureWrapMode::Repeat);
    cm.setWrapModeV(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeU(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeV(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapModeW(), TextureWrapMode::Repeat);
}

TEST(Cubemap, SetWrapModeWOnlySetsWAxis)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeU(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeV(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeW(), TextureWrapMode::Repeat);
    cm.setWrapModeW(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.getWrapMode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeU(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeV(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.getWrapModeW(), TextureWrapMode::Clamp);
}

TEST(Cubemap, GetFilterModeInitiallyReturnsMipmap)
{
    ASSERT_EQ(Cubemap(1, TextureFormat::RGBA32).getFilterMode(), TextureFilterMode::Mipmap);
}

TEST(Cubemap, SetFilterModeUpdatesFilterMode)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.getFilterMode(), TextureFilterMode::Mipmap);
    cm.setFilterMode(TextureFilterMode::Nearest);
    ASSERT_EQ(cm.getFilterMode(), TextureFilterMode::Nearest);
}

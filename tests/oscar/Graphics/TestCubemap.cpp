#include "oscar/Graphics/Cubemap.hpp"

#include "oscar/Utils/EnumHelpers.hpp"

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>


TEST(Cubemap, CanConstruct1x1RGBA32Cubemap)
{
    osc::Cubemap cubemap{1, osc::TextureFormat::RGBA32};
}

TEST(Cubemap, ConstructorThrowsIfGivenZeroWidth)
{
    ASSERT_ANY_THROW({ osc::Cubemap cubemap(0, osc::TextureFormat::RGBA32); });
}

TEST(Cubemap, ConstructorThrowsIfGivenNegativeWidth)
{
    ASSERT_ANY_THROW({ osc::Cubemap cubemap(-5, osc::TextureFormat::RGBA32); });
}

TEST(Cubemap, CanBeCopyConstructed)
{
    osc::Cubemap const source{1, osc::TextureFormat::RGBA32};
    osc::Cubemap{source};
}

static_assert(std::is_nothrow_move_constructible_v<osc::Cubemap>);

TEST(Cubemap, CanBeMoveConstructed)
{
    osc::Cubemap source{1, osc::TextureFormat::RGBA32};
    osc::Cubemap{std::move(source)};
}

TEST(Cubemap, CanBeCopyAssigned)
{
    osc::Cubemap const first{1, osc::TextureFormat::RGBA32};
    osc::Cubemap second{2, osc::TextureFormat::RGB24};
    second = first;

    ASSERT_EQ(second.getWidth(), first.getWidth());
    ASSERT_EQ(second.getTextureFormat(), first.getTextureFormat());
}

static_assert(std::is_nothrow_assignable_v<osc::Cubemap, osc::Cubemap&&>);

TEST(Cubemap, CanBeMoveAssigned)
{
    int32_t const firstWidth = 1;
    osc::TextureFormat const firstFormat = osc::TextureFormat::RGB24;
    osc::Cubemap first{firstWidth, firstFormat};

    int32_t const secondWidth = 2;
    osc::TextureFormat const secondFormat = osc::TextureFormat::RGBA32;
    osc::Cubemap second{secondWidth, secondFormat};
    second = std::move(first);

    ASSERT_NE(firstWidth, secondWidth);
    ASSERT_NE(firstFormat, secondFormat);
    ASSERT_EQ(second.getWidth(), firstWidth);
    ASSERT_EQ(second.getTextureFormat(), firstFormat);
}

static_assert(std::is_nothrow_destructible_v<osc::Cubemap>);

TEST(Cubemap, CanBeReferenceComparedForEquality)
{
    osc::Cubemap const cubemap{1, osc::TextureFormat::RGBA32};

    ASSERT_EQ(cubemap, cubemap);
}

TEST(Cubemap, CopiesCompareEqual)
{
    osc::Cubemap const cubemap{1, osc::TextureFormat::RGBA32};
    osc::Cubemap const copy{cubemap}; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(cubemap, copy);
}

TEST(Cubemap, MutatingACopyMakesItNotEqual)
{
    osc::Cubemap const cubemap{1, osc::TextureFormat::RGBA32};

    osc::Cubemap copy{cubemap};
    std::array<uint8_t, 4> const data = {};
    copy.setPixelData(osc::CubemapFace::PositiveX, data);

    ASSERT_NE(cubemap, copy);
}

TEST(Cubemap, EqualityIsReferenceAndNotValueBased)
{
    // landmine test: this just verifies that equality is
    // really just reference equality, rather than actual
    // value equality (which is better)
    //
    // if the implementation of osc::Cubemap has been updated
    // to enabled value-equality (e.g. by comparing the actual
    // image data or using a strong hashing technique) then
    // this test can be deleted
    osc::Cubemap const a{1, osc::TextureFormat::RGBA32};
    osc::Cubemap const b{1, osc::TextureFormat::RGBA32};

    ASSERT_NE(a, b);
}

TEST(Cubemap, GetWidthReturnsConstructedWidth)
{
    int32_t const width = 4;
    osc::Cubemap const cubemap{width, osc::TextureFormat::RGBA32};

    ASSERT_EQ(cubemap.getWidth(), width);
}

TEST(Cubemap, GetFormatReturnsConstructedFormat)
{
    osc::TextureFormat const format = osc::TextureFormat::RGB24;
    osc::Cubemap const cubemap{1, format};

    ASSERT_EQ(cubemap.getTextureFormat(), format);
}

TEST(Cubemap, SetDataWorksForAnyFaceIfGivenCorrectNumberOfBytes)
{
    osc::TextureFormat const format = osc::TextureFormat::RGBA32;
    constexpr size_t bytesPerPixelForFormat = 4;
    constexpr int32_t width = 5;
    constexpr int32_t nPixels = width*width*bytesPerPixelForFormat;
    std::array<uint8_t, nPixels> const data = {};

    osc::Cubemap cubemap{width, format};

    static_assert(std::is_same_v<std::underlying_type_t<osc::CubemapFace>, int32_t>);
    for (int32_t i = 0; i < static_cast<int32_t>(osc::NumOptions<osc::CubemapFace>()); ++i)
    {
        auto const face = static_cast<osc::CubemapFace>(i);
        cubemap.setPixelData(face, data);
    }
}

TEST(Cubemap, SetDataThrowsIfGivenIncorrectNumberOfBytesForRGBA32)
{
    osc::TextureFormat const format = osc::TextureFormat::RGBA32;
    constexpr size_t incorrectBytesPerPixelForFormat = 3;
    constexpr int32_t width = 5;
    constexpr int32_t nPixels = width*width*incorrectBytesPerPixelForFormat;
    std::array<uint8_t, nPixels> const data = {};

    osc::Cubemap cubemap{width, format};

    static_assert(std::is_same_v<std::underlying_type_t<osc::CubemapFace>, int32_t>);
    for (int32_t i = 0; i < static_cast<int32_t>(osc::NumOptions<osc::CubemapFace>()); ++i)
    {
        auto const face = static_cast<osc::CubemapFace>(i);
        ASSERT_ANY_THROW({ cubemap.setPixelData(face, data); });
    }
}

TEST(Cubemap, SetDataThrowsIfGivenIncorrectNumberOfBytesForRGB24)
{
    osc::TextureFormat const format = osc::TextureFormat::RGB24;
    constexpr size_t incorrectBytesPerPixelForFormat = 4;
    constexpr int32_t width = 5;
    constexpr int32_t nPixels = width*width*incorrectBytesPerPixelForFormat;
    std::array<uint8_t, nPixels> const data = {};

    osc::Cubemap cubemap{width, format};

    static_assert(std::is_same_v<std::underlying_type_t<osc::CubemapFace>, int32_t>);
    for (int32_t i = 0; i < static_cast<int32_t>(osc::NumOptions<osc::CubemapFace>()); ++i)
    {
        auto const face = static_cast<osc::CubemapFace>(i);
        ASSERT_ANY_THROW({ cubemap.setPixelData(face, data); });
    }
}

TEST(Cubemap, SetDataThrowsIfGivenIncorrectNumberOfBytesForWidth)
{
    osc::TextureFormat const format = osc::TextureFormat::RGBA32;
    constexpr size_t bytesPerPixelForFormat = 4;
    constexpr int32_t width = 5;
    constexpr int32_t nPixels = width*width*bytesPerPixelForFormat;
    constexpr int32_t incorrectNPixels = nPixels+3;
    std::array<uint8_t, incorrectNPixels> const data = {};

    osc::Cubemap cubemap{width, format};

    static_assert(std::is_same_v<std::underlying_type_t<osc::CubemapFace>, int32_t>);
    for (int32_t i = 0; i < static_cast<int32_t>(osc::NumOptions<osc::CubemapFace>()); ++i)
    {
        auto const face = static_cast<osc::CubemapFace>(i);
        ASSERT_ANY_THROW({ cubemap.setPixelData(face, data); });
    }
}

TEST(Cubemap, SetPixelDataWorksWithFloatingPointTextureFormats)
{
    [[maybe_unused]] osc::TextureFormat const format = osc::TextureFormat::RGBAFloat;
}

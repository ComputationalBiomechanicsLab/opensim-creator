#include <oscar/Graphics/Cubemap.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

using namespace osc;

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

    ASSERT_EQ(second.width(), first.width());
    ASSERT_EQ(second.texture_format(), first.texture_format());
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
    ASSERT_EQ(second.width(), firstWidth);
    ASSERT_EQ(second.texture_format(), firstFormat);
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
    copy.set_pixel_data(CubemapFace::PositiveX, data);

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

    ASSERT_EQ(cubemap.width(), width);
}

TEST(Cubemap, GetFormatReturnsConstructedFormat)
{
    TextureFormat const format = TextureFormat::RGB24;
    Cubemap const cubemap{1, format};

    ASSERT_EQ(cubemap.texture_format(), format);
}

TEST(Cubemap, SetDataWorksForAnyFaceIfGivenCorrectNumberOfBytes)
{
    TextureFormat const format = TextureFormat::RGBA32;
    constexpr size_t bytesPerPixelForFormat = 4;
    constexpr size_t width = 5;
    constexpr size_t nPixels = width*width*bytesPerPixelForFormat;
    std::array<uint8_t, nPixels> const data = {};

    Cubemap cubemap{width, format};
    for (CubemapFace face : make_option_iterable<CubemapFace>())
    {
        cubemap.set_pixel_data(face, data);
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
    for (CubemapFace face : make_option_iterable<CubemapFace>())
    {
        ASSERT_ANY_THROW({ cubemap.set_pixel_data(face, data); });
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
    for (CubemapFace face : make_option_iterable<CubemapFace>())
    {
        ASSERT_ANY_THROW({ cubemap.set_pixel_data(face, data); });
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
    for (CubemapFace face : make_option_iterable<CubemapFace>())
    {
        ASSERT_ANY_THROW({ cubemap.set_pixel_data(face, data); });
    }
}

TEST(Cubemap, SetPixelDataWorksWithFloatingPointTextureFormats)
{
    [[maybe_unused]] TextureFormat const format = TextureFormat::RGBAFloat;
}

TEST(Cubemap, GetWrapModeInitiallyReturnsRepeatedByDefault)
{
    ASSERT_EQ(Cubemap(1, TextureFormat::RGBA32).wrap_mode(), TextureWrapMode::Repeat);
}

TEST(Cubemap, SetWrapModeUpdatesWrapMode)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Repeat);
    cm.set_wrap_mode(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Clamp);
}

TEST(Cubemap, SetWrapModeSetsAllAxes)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_w(), TextureWrapMode::Repeat);
    cm.set_wrap_mode(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode_u(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode_v(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode_w(), TextureWrapMode::Clamp);
}

TEST(Cubemap, SetWrapModeUSetsUAxisAndGetWrapModeRv)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_w(), TextureWrapMode::Repeat);
    cm.set_wrap_mode_u(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode_u(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_w(), TextureWrapMode::Repeat);
}

TEST(Cubemap, SetWrapModeVOnlySetsVAxis)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_w(), TextureWrapMode::Repeat);
    cm.set_wrap_mode_v(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_v(), TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode_w(), TextureWrapMode::Repeat);
}

TEST(Cubemap, SetWrapModeWOnlySetsWAxis)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_w(), TextureWrapMode::Repeat);
    cm.set_wrap_mode_w(TextureWrapMode::Clamp);
    ASSERT_EQ(cm.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cm.wrap_mode_w(), TextureWrapMode::Clamp);
}

TEST(Cubemap, GetFilterModeInitiallyReturnsMipmap)
{
    ASSERT_EQ(Cubemap(1, TextureFormat::RGBA32).filter_mode(), TextureFilterMode::Mipmap);
}

TEST(Cubemap, SetFilterModeUpdatesFilterMode)
{
    Cubemap cm{1, TextureFormat::RGBA32};
    ASSERT_EQ(cm.filter_mode(), TextureFilterMode::Mipmap);
    cm.set_filter_mode(TextureFilterMode::Nearest);
    ASSERT_EQ(cm.filter_mode(), TextureFilterMode::Nearest);
}

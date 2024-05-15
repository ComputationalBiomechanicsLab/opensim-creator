#include <oscar/Formats/Image.h>

#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/ResourceStream.h>
#include <oscar/Utils/NullOStream.h>

#include <array>
#include <filesystem>

using namespace osc;

TEST(Image, LoadTexture2DFromImageRespectsSRGBColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    Texture2D const rv = load_texture2D_from_image(ResourceStream{path}, ColorSpace::sRGB);

    ASSERT_EQ(rv.color_space(), ColorSpace::sRGB);
}

TEST(Image, LoadTexture2DFromImageRespectsLinearColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    Texture2D const rv = load_texture2D_from_image(ResourceStream{path}, ColorSpace::Linear);

    ASSERT_EQ(rv.color_space(), ColorSpace::Linear);
}

TEST(Image, CanLoadAndThenWriteImage)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";
    Texture2D const rv = load_texture2D_from_image(ResourceStream{path}, ColorSpace::Linear);

    NullOStream out;
    ASSERT_NO_THROW({ write_to_png(rv, out); });
    ASSERT_TRUE(out.was_written_to());
}

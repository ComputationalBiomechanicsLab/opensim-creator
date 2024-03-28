#include <oscar/Formats/Image.h>

#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Platform/AppConfig.h>
#include <oscar/Platform/ResourceStream.h>

#include <array>
#include <filesystem>

using namespace osc;

TEST(Image, LoadTexture2DFromImageRespectsSRGBColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    Texture2D const rv = load_texture2D_from_image(ResourceStream{path}, ColorSpace::sRGB);

    ASSERT_EQ(rv.getColorSpace(), ColorSpace::sRGB);
}

TEST(Image, LoadTexture2DFromImageRespectsLinearColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    Texture2D const rv = load_texture2D_from_image(ResourceStream{path}, ColorSpace::Linear);

    ASSERT_EQ(rv.getColorSpace(), ColorSpace::Linear);
}

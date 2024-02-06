#include <oscar/Formats/Image.hpp>

#include <testoscar/testoscarconfig.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Platform/ResourceStream.hpp>

#include <array>
#include <filesystem>

using namespace osc;

TEST(Image, LoadTexture2DFromImageRespectsSRGBColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    Texture2D const rv = LoadTexture2DFromImage(ResourceStream{path}, ColorSpace::sRGB);

    ASSERT_EQ(rv.getColorSpace(), ColorSpace::sRGB);
}

TEST(Image, LoadTexture2DFromImageRespectsLinearColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    Texture2D const rv = LoadTexture2DFromImage(ResourceStream{path}, ColorSpace::Linear);

    ASSERT_EQ(rv.getColorSpace(), ColorSpace::Linear);
}

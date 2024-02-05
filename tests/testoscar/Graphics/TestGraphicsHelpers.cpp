#include <oscar/Graphics/GraphicsHelpers.hpp>

#include <testoscar/testoscarconfig.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Platform/AppConfig.hpp>

#include <array>
#include <filesystem>

using osc::ColorSpace;
using osc::Texture2D;

TEST(GraphicsHelpers, LoadTexture2DFromImageRespectsSRGBColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    Texture2D const rv = LoadTexture2DFromImage(path, ColorSpace::sRGB);

    ASSERT_EQ(rv.getColorSpace(), ColorSpace::sRGB);
}

TEST(GraphicsHelpers, LoadTexture2DFromImageRespectsLinearColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    Texture2D const rv = LoadTexture2DFromImage(path, ColorSpace::Linear);

    ASSERT_EQ(rv.getColorSpace(), ColorSpace::Linear);
}

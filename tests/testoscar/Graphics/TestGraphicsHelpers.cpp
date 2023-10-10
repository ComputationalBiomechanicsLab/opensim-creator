#include <oscar/Graphics/GraphicsHelpers.hpp>

#include <testoscar/testoscarconfig.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Platform/AppConfig.hpp>

#include <array>
#include <filesystem>

TEST(GraphicsHelpers, LoadTexture2DFromImageRespectsSRGBColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    osc::Texture2D const rv = osc::LoadTexture2DFromImage(path, osc::ColorSpace::sRGB);

    ASSERT_EQ(rv.getColorSpace(), osc::ColorSpace::sRGB);
}

TEST(GraphicsHelpers, LoadTexture2DFromImageRespectsLinearColorSpace)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";

    osc::Texture2D const rv = osc::LoadTexture2DFromImage(path, osc::ColorSpace::Linear);

    ASSERT_EQ(rv.getColorSpace(), osc::ColorSpace::Linear);
}

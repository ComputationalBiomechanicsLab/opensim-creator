#include <oscar/Graphics/GraphicsHelpers.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Platform/AppConfig.hpp>

#include <array>

namespace
{
    osc::AppConfig LoadConfig()
    {
        return osc::AppConfig{"oscar", "tests"};
    }
}

TEST(GraphicsHelpers, LoadTexture2DFromImageRespectsSRGBColorSpace)
{
    auto config = LoadConfig();
    auto path = config.getResourceDir() / "textures" / "awesomeface.png";

    osc::Texture2D const rv = osc::LoadTexture2DFromImage(path, osc::ColorSpace::sRGB);

    ASSERT_EQ(rv.getColorSpace(), osc::ColorSpace::sRGB);
}

TEST(GraphicsHelpers, LoadTexture2DFromImageRespectsLinearColorSpace)
{
    auto config = LoadConfig();
    auto path = config.getResourceDir() / "textures" / "awesomeface.png";

    osc::Texture2D const rv = osc::LoadTexture2DFromImage(path, osc::ColorSpace::Linear);

    ASSERT_EQ(rv.getColorSpace(), osc::ColorSpace::Linear);
}

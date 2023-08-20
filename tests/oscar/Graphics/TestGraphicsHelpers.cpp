#include "oscar/Graphics/GraphicsHelpers.hpp"

#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Platform/Config.hpp"

#include <gtest/gtest.h>

#include <array>

TEST(GraphicsHelpers, LoadTexture2DFromImageRespectsSRGBColorSpace)
{
    auto config = osc::Config::load();
    auto path = config->getResourceDir() / "textures" / "awesomeface.png";

    osc::Texture2D const rv = osc::LoadTexture2DFromImage(path, osc::ColorSpace::sRGB);

    ASSERT_EQ(rv.getColorSpace(), osc::ColorSpace::sRGB);
}

TEST(GraphicsHelpers, LoadTexture2DFromImageRespectsLinearColorSpace)
{
    auto config = osc::Config::load();
    auto path = config->getResourceDir() / "textures" / "awesomeface.png";

    osc::Texture2D const rv = osc::LoadTexture2DFromImage(path, osc::ColorSpace::Linear);

    ASSERT_EQ(rv.getColorSpace(), osc::ColorSpace::Linear);
}

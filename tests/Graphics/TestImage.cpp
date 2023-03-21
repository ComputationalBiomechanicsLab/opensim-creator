#include "src/Graphics/Image.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Platform/Config.hpp"

#include <gtest/gtest.h>
#include <glm/vec2.hpp>

#include <filesystem>
#include <memory>
#include <utility>

TEST(Image, CanLoadImageResource)
{
    auto config = osc::Config::load();
    auto path = config->getResourceDir() / "textures" / "awesomeface.png";
    
    osc::LoadImageFromFile(path);  // shouldn't throw
}

TEST(Image, LoadedImageHasExpectedDimensionsEtc)
{
    std::unique_ptr<osc::Config> config = osc::Config::load();
    std::filesystem::path path = config->getResourceDir() / "textures" / "awesomeface.png";

    osc::Image image = osc::LoadImageFromFile(path);

    glm::ivec2 dimensions = image.getDimensions();
    int numChannels = image.getNumChannels();

    ASSERT_EQ(dimensions, glm::ivec2(512, 512));
    ASSERT_EQ(numChannels, 4);
    ASSERT_EQ(image.getPixelData().size(), dimensions.x * dimensions.y * numChannels);
}

TEST(Image, CanMoveConstruct)
{
    std::unique_ptr<osc::Config> config = osc::Config::load();
    std::filesystem::path path = config->getResourceDir() / "textures" / "awesomeface.png";
    osc::Image src = osc::LoadImageFromFile(path);
    osc::Image image{std::move(src)};

    glm::ivec2 dimensions = image.getDimensions();
    int numChannels = image.getNumChannels();

    ASSERT_EQ(dimensions, glm::ivec2(512, 512));
    ASSERT_EQ(numChannels, 4);
    ASSERT_EQ(image.getPixelData().size(), dimensions.x * dimensions.y * numChannels);
}
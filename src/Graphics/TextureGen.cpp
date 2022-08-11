#include "TextureGen.hpp"

#include "src/Graphics/Image.hpp"
#include "src/Graphics/ImageGen.hpp"

#include <glm/vec2.hpp>


osc::Texture2D osc::GenChequeredFloorTexture()
{
    Image img = GenerateChequeredFloorImage();

    Texture2D rv{static_cast<int>(img.getDimensions().x), static_cast<int>(img.getDimensions().y), img.getPixelData(), img.getNumChannels()};
    rv.setFilterMode(TextureFilterMode::Mipmap);
    rv.setWrapMode(TextureWrapMode::Repeat);

    return rv;
}

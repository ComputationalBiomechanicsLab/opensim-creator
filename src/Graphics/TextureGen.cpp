#include "TextureGen.hpp"

#include "src/Graphics/Image.hpp"
#include "src/Graphics/ImageGen.hpp"

#include <glm/vec2.hpp>


osc::Texture2D osc::GenChequeredFloorTexture()
{
    Image const img = GenerateChequeredFloorImage();

    Texture2D rv
    {
        img.getDimensions(),
        img.getPixelData(),
        img.getNumChannels(),
    };
    rv.setFilterMode(TextureFilterMode::Mipmap);
    rv.setWrapMode(TextureWrapMode::Repeat);

    return rv;
}

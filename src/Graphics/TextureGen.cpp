#include "TextureGen.hpp"

#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Image.hpp"
#include "src/Graphics/ImageGen.hpp"
#include "src/Graphics/TextureFormat.hpp"

#include <glm/vec2.hpp>

#include <optional>


osc::Texture2D osc::GenChequeredFloorTexture()
{
    Image const img = GenerateChequeredFloorImage();
    Texture2D rv = ToTexture2D(img);
    rv.setFilterMode(TextureFilterMode::Mipmap);
    rv.setWrapMode(TextureWrapMode::Repeat);
    return rv;
}

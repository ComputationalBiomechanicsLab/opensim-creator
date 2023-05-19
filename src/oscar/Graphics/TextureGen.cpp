#include "TextureGen.hpp"

#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Image.hpp"
#include "oscar/Graphics/ImageGen.hpp"
#include "oscar/Graphics/TextureFormat.hpp"

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

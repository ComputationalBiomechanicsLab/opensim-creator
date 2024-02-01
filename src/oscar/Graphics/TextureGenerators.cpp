#include "TextureGenerators.hpp"

#include <oscar/Graphics/Color32.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Utils/ObjectRepresentation.hpp>

#include <cstddef>
#include <optional>
#include <vector>

using osc::Texture2D;

Texture2D osc::GenerateChequeredFloorTexture()
{
    constexpr Vec2i chequerDims = {32, 32};
    constexpr Vec2i textureDims = 2 * chequerDims;
    constexpr Color32 onColor = {0xff, 0xff, 0xff, 0xff};
    constexpr Color32 offColor = {0xf3, 0xf3, 0xf3, 0xff};

    std::vector<Color32> pixels;
    pixels.reserve(Area(textureDims));
    for (int y = 0; y < textureDims.y; ++y)
    {
        bool const yOn = (y / chequerDims.y) % 2 == 0;
        for (int x = 0; x < textureDims.x; ++x)
        {
            bool const xOn = (x / chequerDims.x) % 2 == 0;
            pixels.push_back(yOn ^ xOn ? onColor : offColor);
        }
    }

    Texture2D rv
    {
        textureDims,
        TextureFormat::RGBA32,
        ColorSpace::sRGB,
        TextureWrapMode::Repeat,
        TextureFilterMode::Mipmap,
    };
    rv.setPixelData(ViewObjectRepresentations<uint8_t>(pixels));
    return rv;
}

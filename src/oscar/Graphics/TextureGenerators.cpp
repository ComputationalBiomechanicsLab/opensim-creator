#include "TextureGenerators.hpp"

#include <oscar/Graphics/Color32.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/TextureFormat.hpp>

#include <glm/vec2.hpp>

#include <cstddef>
#include <optional>
#include <vector>


osc::Texture2D osc::GenChequeredFloorTexture()
{
    constexpr size_t chequerWidth = 32;
    constexpr size_t chequerHeight = 32;
    constexpr size_t textureWidth = 2 * chequerWidth;
    constexpr size_t textureHeight = 2 * chequerHeight;
    constexpr Color32 onColor = {0xff, 0xff, 0xff, 0xff};
    constexpr Color32 offColor = {0xf3, 0xf3, 0xf3, 0xff};

    std::vector<Color32> pixels(textureWidth * textureHeight);
    for (size_t row = 0; row < textureHeight; ++row)
    {
        size_t const rowStart = row * textureWidth;
        bool const yOn = (row / chequerHeight) % 2 == 0;
        for (size_t col = 0; col < textureWidth; ++col)
        {
            bool const xOn = (col / chequerWidth) % 2 == 0;
            pixels[rowStart + col] = yOn ^ xOn ? onColor : offColor;
        }
    }

    Texture2D rv
    {
        glm::vec2{textureWidth, textureHeight},
        TextureFormat::RGBA32,
        ColorSpace::sRGB,
        TextureWrapMode::Repeat,
        TextureFilterMode::Mipmap,
    };
    rv.setPixelData(nonstd::span<uint8_t const>{&pixels.front().r, sizeof(decltype(pixels)::value_type)*pixels.size()});
    return rv;
}

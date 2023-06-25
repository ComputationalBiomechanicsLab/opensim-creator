#include "TextureGen.hpp"

#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Image.hpp"
#include "oscar/Graphics/Rgb24.hpp"
#include "oscar/Graphics/TextureFormat.hpp"

#include <glm/vec2.hpp>

#include <array>
#include <cstddef>
#include <optional>


osc::Texture2D osc::GenChequeredFloorTexture()
{
    constexpr size_t chequerWidth = 32;
    constexpr size_t chequerHeight = 32;
    constexpr size_t textureWidth = 2 * chequerWidth;
    constexpr size_t textureHeight = 2 * chequerHeight;
    constexpr Rgb24 onColor = {0xff, 0xff, 0xff};
    constexpr Rgb24 offColor = {0xf3, 0xf3, 0xf3};

    std::array<Rgb24, textureWidth * textureHeight> pixels{};
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

    Image const img
    {
        glm::vec2{textureWidth, textureHeight},
        nonstd::span<uint8_t const>{&pixels.front().r, sizeof(pixels)},
        sizeof(Rgb24),  // num channels
        ColorSpace::sRGB,
    };

    Texture2D rv = ToTexture2D(img);
    rv.setFilterMode(TextureFilterMode::Mipmap);
    rv.setWrapMode(TextureWrapMode::Repeat);
    return rv;
}

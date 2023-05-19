#include "ImageGen.hpp"

#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Rgb24.hpp"

#include <array>
#include <cstddef>

osc::Image osc::GenerateChequeredFloorImage()
{
    constexpr size_t chequerWidth = 32;
    constexpr size_t chequerHeight = 32;
    constexpr size_t textureWidth = 2 * chequerWidth;
    constexpr size_t textureHeight = 2 * chequerHeight;
    constexpr Rgb24 onColor = {0xff, 0xff, 0xff};
    constexpr Rgb24 offColor = {0xf3, 0xf3, 0xf3};

    std::array<Rgb24, textureWidth * textureHeight> pixels;
    for (size_t row = 0; row < textureHeight; ++row)
    {
        size_t const rowStart = row * textureWidth;
        bool yOn = (row / chequerHeight) % 2 == 0;
        for (size_t col = 0; col < textureWidth; ++col)
        {
            bool const xOn = (col / chequerWidth) % 2 == 0;
            pixels[rowStart + col] = yOn ^ xOn ? onColor : offColor;
        }
    }

    return Image
    {
        glm::vec2{textureWidth, textureHeight},
        nonstd::span<uint8_t const>{&pixels.front().r, sizeof(pixels)},
        sizeof(Rgb24),  // num channels
        ColorSpace::sRGB,
    };
}
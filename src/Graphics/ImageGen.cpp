#include "ImageGen.hpp"

#include <array>
#include <cstddef>

osc::Image osc::GenerateChequeredFloorImage()
{
    constexpr size_t chequer_width = 32;
    constexpr size_t chequer_height = 32;
    constexpr size_t w = 2 * chequer_width;
    constexpr size_t h = 2 * chequer_height;

    struct Rgb { unsigned char r, g, b; };
    constexpr Rgb on_color = {0xff, 0xff, 0xff};
    constexpr Rgb off_color = {0xf3, 0xf3, 0xf3};

    std::array<Rgb, w * h> pixels;
    for (size_t row = 0; row < h; ++row)
    {
        size_t row_start = row * w;
        bool y_on = (row / chequer_height) % 2 == 0;
        for (size_t col = 0; col < w; ++col)
        {
            bool x_on = (col / chequer_width) % 2 == 0;
            pixels[row_start + col] = y_on ^ x_on ? on_color : off_color;
        }
    }

    nonstd::span<uint8_t> rawPixels{&pixels.front().r, w * h * 3};

    return Image{glm::ivec2(w, h), rawPixels, 3};
}
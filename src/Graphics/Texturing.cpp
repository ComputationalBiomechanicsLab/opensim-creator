#include "Texturing.hpp"

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/Image.hpp"
#include "src/Graphics/ImageFlags.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>


// public API

gl::Texture2D osc::GenChequeredFloorTexture()
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

    gl::Texture2D rv;
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(rv.type, rv.handle());
    glTexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(rv.type);
    gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return rv;
}

osc::ImageTexture osc::LoadImageAsTexture(std::filesystem::path const& path, ImageFlags flags)
{
    Image img = Image::Load(path, flags);
    int numChannels = img.getNumChannels();

    GLenum internalFormat;
    GLenum format;
    switch (numChannels)
    {
    case 1:
        internalFormat = GL_RED;
        format = GL_RED;
        break;
    case 3:
        internalFormat = GL_RGB;
        format = GL_RGB;
        break;
    case 4:
        internalFormat = GL_RGBA;
        format = GL_RGBA;
        break;
    default:
    {
        std::stringstream msg;
        msg << path << ": error: contains " << numChannels
            << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }
    }

    glm::ivec2 dims = img.getDimensions();

    gl::Texture2D t;
    gl::BindTexture(t.type, t.handle());
    gl::TexImage2D(t.type, 0, internalFormat, dims.x, dims.y, 0, format, GL_UNSIGNED_BYTE, img.getPixelData().data());
    glGenerateMipmap(t.type);

    return ImageTexture{std::move(t), dims, numChannels};
}

#include "Texturing.hpp"

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/Image.hpp"
#include "src/Graphics/ImageFlags.hpp"
#include "src/Graphics/ImageGen.hpp"

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
    Image img = GenerateChequeredFloorImage();

    gl::Texture2D rv;
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(rv.type, rv.handle());
    glTexImage2D(rv.type, 0, GL_RGB, img.getDimensions().x, img.getDimensions().y, 0, GL_RGB, GL_UNSIGNED_BYTE, img.getPixelData().data());
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

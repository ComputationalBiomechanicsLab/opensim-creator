#pragma once

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/ImageFlags.hpp"

#include <glm/vec2.hpp>

#include <filesystem>

namespace osc
{
    // generate a chequered floor texture
    //
    // this is typically used as a default scene floor for visualization
    gl::Texture2D GenChequeredFloorTexture();

    // an image loaded onto the GPU, plus CPU-side metadata (dimensions, channels)
    struct ImageTexture final {
        gl::Texture2D Texture;
        glm::ivec2 Dimensions;
        int NumChannels;  // in most cases, 3 == RGB, 4 == RGBA
    };

    // read an image file (.PNG, .JPEG, etc.) directly into an OpenGL (GPU) texture
    ImageTexture LoadImageAsTexture(std::filesystem::path const&, ImageFlags = ImageFlags_None);

    // read 6 image files into a single OpenGL cubemap (GL_TEXTURE_CUBE_MAP)
    //
    // useful for skyboxes, precomputed point-shadow maps, etc.
    gl::TextureCubemap LoadCubemapAsCubemapTexture(
        std::filesystem::path const& posX,
        std::filesystem::path const& negX,
        std::filesystem::path const& posY,
        std::filesystem::path const& negY,
        std::filesystem::path const& posZ,
        std::filesystem::path const& negZ,
        ImageFlags = ImageFlags_None);
}

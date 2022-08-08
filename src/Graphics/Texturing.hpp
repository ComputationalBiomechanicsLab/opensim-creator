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
}

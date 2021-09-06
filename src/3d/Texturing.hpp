#pragma once

#include "src/3d/Gl.hpp"

namespace osc {

    // texturing

    // generate a chequered floor texture
    //
    // this is typically used as a default scene floor for visualization
    gl::Texture2D genChequeredFloorTexture();

    // flags for loading textures from disk
    enum TexFlag {
        TexFlag_None = 0,
        TexFlag_SRGB = 1,

        // BEWARE: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but can cause surprising behavior if the pixels represent vectors
        //
        // therefore, if you are flipping (e.g.) normal maps, you may *also* need
        // to flip the pixel content appropriately (e.g. if RGB represents XYZ then
        // you'll need to negate each G)
        TexFlag_FlipPixelsVertically = 2,
    };

    // an image loaded onto the GPU, plus CPU-side metadata (dimensions, channels)
    struct ImageTexture final {
        gl::Texture2D texture;
        int width;
        int height;
        int channels;  // in most cases, 3 == RGB, 4 == RGBA
    };

    // read an image file (.PNG, .JPEG, etc.) directly into an OpenGL (GPU) texture
    ImageTexture loadImageAsTexture(char const* path, TexFlag = TexFlag_None);

    // read 6 image files into a single OpenGL cubemap (GL_TEXTURE_CUBE_MAP)
    //
    // useful for skyboxes, precomputed point-shadow maps, etc.
    gl::TextureCubemap loadCubemapAsCubemapTexture(
        char const* pathPosX,
        char const* pathNegX,
        char const* pathPosY,
        char const* pathNegY,
        char const* pathPosZ,
        char const* pathNegZ);
}

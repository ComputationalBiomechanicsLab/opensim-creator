#pragma once

#include "src/3d/gl.hpp"

namespace osc {

    // texturing

    // generate a chequered floor texture
    //
    // this is typically used as a default scene floor for visualization
    gl::Texture_2d generate_chequered_floor_texture();

    // flags for loading textures from disk
    enum Tex_flags {
        TexFlag_None = 0,
        TexFlag_SRGB = 1,

        // BEWARE: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but can cause surprising behavior if the pixels represent vectors
        //
        // therefore, if you are flipping (e.g.) normal maps, you may *also* need
        // to flip the pixel content appropriately (e.g. if RGB represents XYZ then
        // you'll need to negate each G)
        TexFlag_Flip_Pixels_Vertically = 2,
    };

    // an image loaded onto the GPU, plus CPU-side metadata (dimensions, channels)
    struct Image_texture final {
        gl::Texture_2d texture;
        int width;
        int height;
        int channels;  // in most cases, 3 == RGB, 4 == RGBA
    };

    // read an image file (.PNG, .JPEG, etc.) directly into an OpenGL (GPU) texture
    Image_texture load_image_as_texture(char const* path, Tex_flags = TexFlag_None);

    // read 6 image files into a single OpenGL cubemap (GL_TEXTURE_CUBE_MAP)
    //
    // useful for skyboxes, precomputed point-shadow maps, etc.
    gl::Texture_cubemap load_cubemap(
        char const* path_pos_x,
        char const* path_neg_x,
        char const* path_pos_y,
        char const* path_neg_y,
        char const* path_pos_z,
        char const* path_neg_z);
}

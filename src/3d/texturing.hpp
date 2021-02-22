#pragma once

#include "gl.hpp"

namespace osmv {
    gl::Texture_2d generate_chequered_floor_texture();

    enum Tex_flags {
        TexFlag_None = 0,
        TexFlag_SRGB = 1,

        // beware: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but causes surprising behavior if the pixels represent vectors (e.g.
        // normal maps)
        TexFlag_Flip_Pixels_Vertically = 2,
    };

    // read an image file into an OpenGL 2D texture
    gl::Texture_2d load_tex(char const* path, Tex_flags = TexFlag_None);

    // read 6 image files into a single OpenGL cubemap (GL_TEXTURE_CUBE_MAP)
    gl::Texture_cubemap load_cubemap(
        char const* path_pos_x,
        char const* path_neg_x,
        char const* path_pos_y,
        char const* path_neg_y,
        char const* path_pos_z,
        char const* path_neg_z);
}

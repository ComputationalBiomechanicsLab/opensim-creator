#include "texturing.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <GL/glew.h>
#include <stb_image.h>

#include <array>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>

using std::literals::operator""s;

gl::Texture_2d osmv::generate_chequered_floor_texture() {
    struct Rgb {
        unsigned char r, g, b;
    };
    constexpr size_t chequer_width = 32;
    constexpr size_t chequer_height = 32;
    constexpr size_t w = 2 * chequer_width;
    constexpr size_t h = 2 * chequer_height;
    constexpr Rgb on_color = {0xe5, 0xe5, 0xe5};
    constexpr Rgb off_color = {0xde, 0xde, 0xde};

    std::array<Rgb, w * h> pixels;
    for (size_t row = 0; row < h; ++row) {
        size_t row_start = row * w;
        bool y_on = (row / chequer_height) % 2 == 0;
        for (size_t col = 0; col < w; ++col) {
            bool x_on = (col / chequer_width) % 2 == 0;
            pixels[row_start + col] = y_on ^ x_on ? on_color : off_color;
        }
    }

    gl::Texture_2d rv;
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(rv.type, rv);
    glTexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(rv.type);
    gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return rv;
}

namespace stbi {
    struct Image final {
        int width;
        int height;
        int nrChannels;
        unsigned char* data;

        Image(char const* path) : data{stbi_load(path, &width, &height, &nrChannels, 0)} {
            if (data == nullptr) {
                throw std::runtime_error{"stbi_load failed for '"s + path + "' : " + stbi_failure_reason()};
            }
        }
        Image(Image const&) = delete;
        Image(Image&&) = delete;
        Image& operator=(Image const&) = delete;
        Image& operator=(Image&&) = delete;
        ~Image() noexcept {
            stbi_image_free(data);
        }
    };
}

gl::Texture_2d osmv::load_tex(char const* path, Tex_flags flags) {
    gl::Texture_2d t;

    if (flags & TexFlag_Flip_Pixels_Vertically) {
        stbi_set_flip_vertically_on_load(true);
    }
    auto img = stbi::Image{path};
    if (flags & TexFlag_Flip_Pixels_Vertically) {
        stbi_set_flip_vertically_on_load(false);
    }

    GLenum internalFormat;
    GLenum format;
    if (img.nrChannels == 1) {
        internalFormat = GL_RED;
        format = GL_RED;
    } else if (img.nrChannels == 3) {
        internalFormat = flags & TexFlag_SRGB ? GL_SRGB : GL_RGB;
        format = GL_RGB;
    } else if (img.nrChannels == 4) {
        internalFormat = flags & TexFlag_SRGB ? GL_SRGB_ALPHA : GL_RGBA;
        format = GL_RGBA;
    } else {
        std::stringstream msg;
        msg << path << ": error: contains " << img.nrChannels
            << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    gl::BindTexture(t.type, t);
    gl::TexImage2D(t.type, 0, internalFormat, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.data);
    glGenerateMipmap(t.type);

    return t;
}

// helper method: load a file into an image and send it to OpenGL
static void load_cubemap_surface(char const* path, GLenum target) {
    auto img = stbi::Image{path};

    GLenum format;
    if (img.nrChannels == 1) {
        format = GL_RED;
    } else if (img.nrChannels == 3) {
        format = GL_RGB;
    } else if (img.nrChannels == 4) {
        format = GL_RGBA;
    } else {
        std::stringstream msg;
        msg << path << ": error: contains " << img.nrChannels
            << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    glTexImage2D(target, 0, format, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.data);
}

gl::Texture_cubemap osmv::load_cubemap(
    char const* path_pos_x,
    char const* path_neg_x,
    char const* path_pos_y,
    char const* path_neg_y,
    char const* path_pos_z,
    char const* path_neg_z) {

    stbi_set_flip_vertically_on_load(false);

    gl::Texture_cubemap rv;
    gl::BindTexture(rv);

    load_cubemap_surface(path_pos_x, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    load_cubemap_surface(path_neg_x, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    load_cubemap_surface(path_pos_y, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    load_cubemap_surface(path_neg_y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    load_cubemap_surface(path_pos_z, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    load_cubemap_surface(path_neg_z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    /**
     * From: https://learnopengl.com/Advanced-OpenGL/Cubemaps
     *
     * Don't be scared by the GL_TEXTURE_WRAP_R, this simply sets the wrapping
     * method for the texture's R coordinate which corresponds to the texture's
     * 3rd dimension (like z for positions). We set the wrapping method to
     * GL_CLAMP_TO_EDGE since texture coordinates that are exactly between two
     * faces may not hit an exact face (due to some hardware limitations) so
     * by using GL_CLAMP_TO_EDGE OpenGL always returns their edge values
     * whenever we sample between faces.
     */
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return rv;
}

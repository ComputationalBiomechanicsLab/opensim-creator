#include "texturing.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <optional>
#include <sstream>

namespace stbi {
    // an image loaded via STBI
    struct Image final {
        int width;
        int height;

        // number of color channels in the image
        //
        // assume one byte per channel
        int channels;

        // raw data, containing intercalated color channels, e.g.:
        //
        // [c0, c1, c2, c3, c0, c1, c2, c3]
        //
        // or, more directly:
        //
        // [R, G, B, A, R, G, B, A]
        //
        // although it's more "correct" better to think of it in terms of
        // channels, because some images aren't color (e.g. greyscale,
        // heightmaps, normal maps)
        unsigned char* data;

        // if empty, use `failure_reason` to retrieve the reason why
        static std::optional<Image> load(char const* path) {
            int width;
            int height;
            int channels;
            unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

            if (data) {
                return Image{width, height, channels, data};
            } else {
                return std::nullopt;
            }
        }

    private:
        // used internally by load(path)
        Image(int w, int h, int c, unsigned char* ptr) :
            width{w}, height{h}, channels{c}, data{ptr} {
        }

    public:
        Image(Image const&) = delete;

        constexpr Image(Image&& tmp) noexcept :
            width{tmp.width},
            height{tmp.height},
            channels{tmp.channels},
            data{tmp.data} {

            tmp.data = nullptr;
        }

        ~Image() noexcept {
            if (data) {
                stbi_image_free(data);
            }
        }

        Image& operator=(Image const&) = delete;

        constexpr Image& operator=(Image&& tmp) noexcept {
            width = tmp.width;
            height = tmp.height;
            channels = tmp.channels;

            unsigned char* ptr = data;
            data = tmp.data;
            tmp.data = ptr;

            return *this;
        }
    };
}

// helper method: load a file into an image and send it to OpenGL
static void load_cubemap_surface(char const* path, GLenum target) {
    auto img = stbi::Image::load(path);

    if (!img) {
        std::stringstream ss;
        ss << path << ": error loading cubemap surface: " << stbi_failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }

    GLenum format;
    if (img->channels == 1) {
        format = GL_RED;
    } else if (img->channels == 3) {
        format = GL_RGB;
    } else if (img->channels == 4) {
        format = GL_RGBA;
    } else {
        std::stringstream msg;
        msg << path << ": error: contains " << img->channels
            << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    glTexImage2D(target, 0, format, img->width, img->height, 0, format, GL_UNSIGNED_BYTE, img->data);
}


// public API

gl::Texture_2d osc::generate_chequered_floor_texture() {
    constexpr size_t chequer_width = 32;
    constexpr size_t chequer_height = 32;
    constexpr size_t w = 2 * chequer_width;
    constexpr size_t h = 2 * chequer_height;

    struct Rgb { unsigned char r, g, b; };
    constexpr Rgb on_color = {0xff, 0xff, 0xff};
    constexpr Rgb off_color = {0xf5, 0xf5, 0xf5};

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
    gl::BindTexture(rv.type, rv.handle());
    glTexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(rv.type);
    gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return rv;
}

osc::Image_texture osc::load_image_as_texture(char const* path, Tex_flags flags) {
    gl::Texture_2d t;

    if (flags & TexFlag_Flip_Pixels_Vertically) {
        stbi_set_flip_vertically_on_load(true);
    }

    auto img = stbi::Image::load(path);

    if (!img) {
        std::stringstream ss;
        ss << path << ": error loading image: " << stbi_failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }

    if (flags & TexFlag_Flip_Pixels_Vertically) {
        stbi_set_flip_vertically_on_load(false);
    }

    GLenum internalFormat;
    GLenum format;
    if (img->channels == 1) {
        internalFormat = GL_RED;
        format = GL_RED;
    } else if (img->channels == 3) {
        internalFormat = flags & TexFlag_SRGB ? GL_SRGB : GL_RGB;
        format = GL_RGB;
    } else if (img->channels == 4) {
        internalFormat = flags & TexFlag_SRGB ? GL_SRGB_ALPHA : GL_RGBA;
        format = GL_RGBA;
    } else {
        std::stringstream msg;
        msg << path << ": error: contains " << img->channels
            << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    gl::BindTexture(t.type, t.handle());
    gl::TexImage2D(t.type, 0, internalFormat, img->width, img->height, 0, format, GL_UNSIGNED_BYTE, img->data);
    glGenerateMipmap(t.type);

    return Image_texture{std::move(t), img->width, img->height, img->channels};
}

gl::Texture_cubemap osc::load_cubemap(
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

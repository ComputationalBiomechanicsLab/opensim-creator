#include "gl_extensions.hpp"

// stbi for image loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// glm for maths
#include <glm/gtc/type_ptr.hpp>

// stdlib
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

using std::literals::operator""s;

namespace stbi {
    struct Image final {
        int width;
        int height;
        int nrChannels;
        unsigned char* data;

        Image(char const* path) :
            data{stbi_load(path, &width, &height, &nrChannels, 0)} {
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

gl::Vertex_shader gl::CompileVertexShader(char const* src) {
    auto s = gl::CreateVertexShader();
    ShaderSource(s, src);
    CompileShader(s);
    return s;
}

gl::Fragment_shader gl::CompileFragmentShader(char const* src) {
    auto s = gl::CreateFragmentShader();
    ShaderSource(s, src);
    CompileShader(s);
    return s;
}

gl::Geometry_shader gl::CompileGeometryShader(char const* src) {
    auto s = gl::CreateGeometryShader();
    ShaderSource(s, src);
    CompileShader(s);
    return s;
}

// convenience helper
gl::Program gl::CreateProgramFrom(Vertex_shader const& vs,
                                  Fragment_shader const& fs) {
    auto p = CreateProgram();
    AttachShader(p, vs);
    AttachShader(p, fs);
    LinkProgram(p);
    return p;
}

gl::Program gl::CreateProgramFrom(Vertex_shader const& vs,
                                  Fragment_shader const& fs,
                                  Geometry_shader const& gs) {
    auto p = CreateProgram();
    AttachShader(p, vs);
    AttachShader(p, gs);
    AttachShader(p, fs);
    LinkProgram(p);
    return p;
}

static std::string slurp_file(const char* path) {
    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f.open(path, std::ios::binary | std::ios::in);

    std::stringstream ss;
    ss << f.rdbuf();

    return ss.str();
}

// asserts there are no current OpenGL errors (globally)
void gl::assert_no_errors(char const* label) {
    static auto to_string = [](GLubyte const* err_string) {
        return std::string{reinterpret_cast<char const*>(err_string)};
    };

    GLenum err = glGetError();
    if (err == GL_NO_ERROR) {
        return;
    }

    std::vector<GLenum> errors;

    do {
        errors.push_back(err);
    } while ((err = glGetError()) != GL_NO_ERROR);

    std::stringstream msg;
    msg << label << " failed";
    if (errors.size() == 1) {
        msg << ": ";
    } else {
        msg << " with " << errors.size() << " errors: ";
    }
    for (auto it = errors.begin(); it != errors.end()-1; ++it) {
        msg << to_string(gluErrorString(*it)) << ", ";
    }
    msg << to_string(gluErrorString(errors.back()));

    throw std::runtime_error{msg.str()};
}

gl::Vertex_shader gl::CompileVertexShaderFile(char const* path) {
    try {
        return CompileVertexShader(slurp_file(path).c_str());
    } catch (std::exception const& e) {
        std::stringstream ss;
        ss << path;
        ss << ": cannot compile vertex shader: ";
        ss << e.what();
        throw std::runtime_error{std::move(ss).str()};
    }
}

gl::Fragment_shader gl::CompileFragmentShaderFile(char const* path) {
    try {
        return CompileFragmentShader(slurp_file(path).c_str());
    } catch (std::exception const& e) {
        std::stringstream ss;
        ss << path;
        ss << ": cannot compile fragment shader: ";
        ss << e.what();
        throw std::runtime_error{std::move(ss).str()};
    }
}

gl::Geometry_shader gl::CompileGeometryShaderFile(char const* path) {
    try {
        return CompileGeometryShader(slurp_file(path).c_str());
    } catch (std::exception const& e) {
        std::stringstream ss;
        ss << path;
        ss << ": cannot compile geometry shader: ";
        ss << e.what();
        throw std::runtime_error{std::move(ss).str()};
    }
}

gl::Texture_2d gl::flipped_and_mipmapped_texture(char const* path, bool srgb) {
    auto t = gl::GenTexture2d();
    auto img = stbi::Image{path};

    GLenum internalFormat;
    GLenum format;
    if (img.nrChannels == 1) {
        internalFormat = GL_RED;
        format = GL_RED;
    } else if (img.nrChannels == 3) {
        internalFormat = srgb ? GL_SRGB : GL_RGB;
        format = GL_RGB;
    } else if (img.nrChannels == 4) {
        internalFormat = srgb ? GL_SRGB_ALPHA : GL_RGBA;
        format = GL_RGBA;
    } else {
        std::stringstream msg;
        msg << path << ": error: contains " << img.nrChannels << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    stbi_set_flip_vertically_on_load(true);

    gl::BindTexture(t.type, t);
    glTexImage2D(t.type,
                 0,
                 internalFormat,
                 img.width,
                 img.height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 img.data);
    glGenerateMipmap(t.type);

    return t;
}

gl::Texture_2d gl::nonflipped_and_mipmapped_texture(char const* path) {
    stbi_set_flip_vertically_on_load(false);

    auto t = gl::GenTexture2d();
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
        msg << path << ": error: contains " << img.nrChannels << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    gl::BindTexture(t.type, t);
    glTexImage2D(t.type,
                 0,
                 format,
                 img.width,
                 img.height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 img.data);
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
        msg << path << ": error: contains " << img.nrChannels << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    glTexImage2D(target, 0, format, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.data);
};

gl::Texture_cubemap gl::read_cubemap(
        char const* path_pos_x,
        char const* path_neg_x,
        char const* path_pos_y,
        char const* path_neg_y,
        char const* path_pos_z,
        char const* path_neg_z) {
    stbi_set_flip_vertically_on_load(false);

    gl::Texture_cubemap rv = gl::GenTextureCubemap();
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

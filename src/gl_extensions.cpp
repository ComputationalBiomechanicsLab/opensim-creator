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
void gl::assert_no_errors(char const* func) {
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
    msg << func << " failed";
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
    return CompileVertexShader(slurp_file(path).c_str());
}

gl::Fragment_shader gl::CompileFragmentShaderFile(char const* path) {
    return CompileFragmentShader(slurp_file(path).c_str());
}

gl::Geometry_shader gl::CompileGeometryShaderFile(char const* path) {
    return CompileGeometryShader(slurp_file(path).c_str());
}

gl::Texture_2d gl::mipmapped_texture(char const* path) {
    auto t = gl::GenTexture2d();
    auto img = stbi::Image{path};

    stbi_set_flip_vertically_on_load(true);

    gl::BindTexture(t.type, t);
    glTexImage2D(t.type,
                 0,
                 img.nrChannels == 3 ? GL_RGB : GL_RGBA,
                 img.width,
                 img.height,
                 0,
                 img.nrChannels == 3 ? GL_RGB : GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 img.data);
    glGenerateMipmap(t.type);

    return t;
}

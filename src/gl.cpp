#include "gl.hpp"

#include <sstream>
#include <vector>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

using std::literals::operator""s;

void gl::CompileShader(Shader& sh) {
    glCompileShader(sh);

    // check for compile errors
    GLint params = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &params);

    if (params == GL_TRUE) {
        return;
    }

    // else: there were compile errors

    GLint log_len = 0;
    glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &log_len);

    std::vector<GLchar> errmsg(log_len);
    glGetShaderInfoLog(sh, log_len, &log_len, errmsg.data());

    std::stringstream ss;
    ss << errmsg.data();
    throw std::runtime_error{"gl::CompileShader failed: "s + ss.str()};
}

void gl::LinkProgram(gl::Program& prog) {
    glLinkProgram(prog);

    // check for link errors
    GLint link_status = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &link_status);

    if (link_status == GL_TRUE) {
        return;
    }

    // else: there were link errors
    GLint log_len = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);

    std::vector<GLchar> errmsg(log_len);
    glGetProgramInfoLog(prog, static_cast<GLsizei>(errmsg.size()), nullptr, errmsg.data());

    std::stringstream ss;
    ss << "OpenGL: glLinkProgram() failed: ";
    ss << errmsg.data();
    throw std::runtime_error{ss.str()};
}

std::string gl::slurp(std::filesystem::path const& path) {
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

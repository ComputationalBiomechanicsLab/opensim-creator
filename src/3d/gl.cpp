#include "gl.hpp"

#include "src/assertions.hpp"
#include "src/utils/os.hpp"

#include <GL/glew.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

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

    OSMV_ASSERT_ALWAYS(log_len >= 0);

    std::vector<GLchar> errmsg(static_cast<size_t>(log_len));
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

    OSMV_ASSERT_ALWAYS(log_len >= 0);

    std::vector<GLchar> errmsg(static_cast<size_t>(log_len));
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

static char const* to_c_str(GLenum err) {
    // string is iso 8859 encoded
    return reinterpret_cast<char const*>(gluErrorString(err));
}

// asserts there are no current OpenGL errors (globally)
void gl::assert_no_errors(char const* comment, char const* file, int line, char const* func) {
    GLenum err = glGetError();
    if (err == GL_NO_ERROR) {
        return;
    }

    osmv::log::error("OpenGL error detected: backtrace:");
    osmv::write_backtrace_to_log(osmv::log::level::err);
    osmv::log::error("throwing the OpenGL error as an exception");

    std::vector<GLenum> errors;

    do {
        errors.push_back(err);
    } while ((err = glGetError()) != GL_NO_ERROR);

    std::stringstream msg;

    msg << file << ':' << line << ": opengl error occurred:\n"
        << "    file = " << file << "\n    line = " << line << "\n    func = " << func << "\n    comment = " << comment
        << '\n';

    if (errors.size() == 1) {
        msg << "    message = " << to_c_str(errors.front());
    } else {
        int i = 0;
        for (GLenum e : errors) {
            msg << "    message (" << i++ << ") = " << to_c_str(e) << '\n';
        }
    }

    throw std::runtime_error{std::move(msg).str()};
}

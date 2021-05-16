#include "gl.hpp"

#include <sstream>
#include <vector>

using std::literals::operator""s;

static std::vector<char const*> current_errors;

char const* gl::Opengl_exception::what() const noexcept {
    return msg.c_str();
}

void gl::CompileFromSource(Shader_handle const& s, const char* src) {
    glShaderSource(s.get(), 1, &src, nullptr);
    glCompileShader(s.get());

    // check for compile errors
    GLint params = GL_FALSE;
    glGetShaderiv(s.get(), GL_COMPILE_STATUS, &params);

    if (params == GL_TRUE) {
        return;
    }

    // else: there were compile errors

    GLint log_len = 0;
    glGetShaderiv(s.get(), GL_INFO_LOG_LENGTH, &log_len);

    std::vector<GLchar> errmsg(log_len);
    glGetShaderInfoLog(s.get(), log_len, &log_len, errmsg.data());

    std::stringstream ss;
    ss << errmsg.data();
    throw std::runtime_error{"gl::CompileShader failed: "s + ss.str()};
}

void gl::LinkProgram(gl::Program& prog) {
    glLinkProgram(prog.get());

    // check for link errors
    GLint link_status = GL_FALSE;
    glGetProgramiv(prog.get(), GL_LINK_STATUS, &link_status);

    if (link_status == GL_TRUE) {
        return;
    }

    // else: there were link errors
    GLint log_len = 0;
    glGetProgramiv(prog.get(), GL_INFO_LOG_LENGTH, &log_len);

    std::vector<GLchar> errmsg(log_len);
    glGetProgramInfoLog(prog.get(), static_cast<GLsizei>(errmsg.size()), nullptr, errmsg.data());

    std::stringstream ss;
    ss << "OpenGL: glLinkProgram() failed: ";
    ss << errmsg.data();
    throw std::runtime_error{ss.str()};
}

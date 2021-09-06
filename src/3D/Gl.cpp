#include "Gl.hpp"

#include <sstream>
#include <vector>

char const* gl::OpenGlException::what() const noexcept {
    return m_Msg.c_str();
}

void gl::CompileFromSource(ShaderHandle const& s, const char* src) {
    glShaderSource(s.get(), 1, &src, nullptr);
    glCompileShader(s.get());

    // check for compile errors
    GLint params = GL_FALSE;
    glGetShaderiv(s.get(), GL_COMPILE_STATUS, &params);

    if (params == GL_TRUE) {
        return;
    }

    // else: there were compile errors

    GLint logLen = 0;
    glGetShaderiv(s.get(), GL_INFO_LOG_LENGTH, &logLen);

    std::vector<GLchar> errMessageBytes(logLen);
    glGetShaderInfoLog(s.get(), logLen, &logLen, errMessageBytes.data());

    std::stringstream ss;
    ss << "gl::CompilesShader failed: " << errMessageBytes.data();
    throw std::runtime_error{std::move(ss).str()};
}

void gl::LinkProgram(gl::Program& prog) {
    glLinkProgram(prog.get());

    // check for link errors
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(prog.get(), GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_TRUE) {
        return;
    }

    // else: there were link errors
    GLint logLen = 0;
    glGetProgramiv(prog.get(), GL_INFO_LOG_LENGTH, &logLen);

    std::vector<GLchar> errMessageBytes(logLen);
    glGetProgramInfoLog(prog.get(), static_cast<GLsizei>(errMessageBytes.size()), nullptr, errMessageBytes.data());

    std::stringstream ss;
    ss << "OpenGL: glLinkProgram() failed: ";
    ss << errMessageBytes.data();
    throw std::runtime_error{ss.str()};
}

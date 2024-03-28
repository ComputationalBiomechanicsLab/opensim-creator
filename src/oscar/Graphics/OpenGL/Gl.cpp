#include "Gl.h"

#include <GL/glew.h>

#include <sstream>
#include <stdexcept>
#include <vector>

void osc::gl::compile_from_source(const ShaderHandle& s, const GLchar* src)
{
    glShaderSource(s.get(), 1, &src, nullptr);
    glCompileShader(s.get());

    // check for compile errors
    GLint params = GL_FALSE;
    glGetShaderiv(s.get(), GL_COMPILE_STATUS, &params);

    if (params == GL_TRUE) {
        return;
    }

    // else: there were compile errors

    GLint log_length = 0;
    glGetShaderiv(s.get(), GL_INFO_LOG_LENGTH, &log_length);

    std::vector<GLchar> error_message_bytes(log_length);
    glGetShaderInfoLog(s.get(), log_length, &log_length, error_message_bytes.data());

    std::stringstream ss;
    ss << "glCompileShader failed: " << error_message_bytes.data();
    throw std::runtime_error{std::move(ss).str()};
}

void osc::gl::link_program(gl::Program& prog)
{
    glLinkProgram(prog.get());

    // check for link errors
    GLint link_status = GL_FALSE;
    glGetProgramiv(prog.get(), GL_LINK_STATUS, &link_status);

    if (link_status == GL_TRUE) {
        return;
    }

    // else: there were link errors
    GLint log_length = 0;
    glGetProgramiv(prog.get(), GL_INFO_LOG_LENGTH, &log_length);

    std::vector<GLchar> error_message_bytes(log_length);
    glGetProgramInfoLog(prog.get(), static_cast<GLsizei>(error_message_bytes.size()), nullptr, error_message_bytes.data());

    std::stringstream ss;
    ss << "OpenGL: glLinkProgram() failed: " << error_message_bytes.data();
    throw std::runtime_error{ss.str()};
}

#include "Gl.h"

#include <glad/glad.h>

#include <liboscar/Utils/Assertions.h>

#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

void osc::gl::compile_from_source(const ShaderHandle& shader_handle, std::string_view shader_src)
{
    OSC_ASSERT_ALWAYS(not shader_src.empty() && "empty source code passed to the shader compiler");
    const GLchar* shader_src_ptr = shader_src.data();
    const auto shader_src_length = static_cast<GLint>(shader_src.size());
    glShaderSource(shader_handle.get(), 1, &shader_src_ptr, &shader_src_length);
    glCompileShader(shader_handle.get());

    // check for compile errors
    GLint params = GL_FALSE;
    glGetShaderiv(shader_handle.get(), GL_COMPILE_STATUS, &params);

    if (params == GL_TRUE) {
        return;
    }

    // else: there were compile errors

    GLint log_length = 0;
    glGetShaderiv(shader_handle.get(), GL_INFO_LOG_LENGTH, &log_length);

    std::vector<GLchar> error_message_bytes(log_length);
    glGetShaderInfoLog(shader_handle.get(), log_length, &log_length, error_message_bytes.data());

    std::stringstream ss;
    ss << "glCompileShader failed: " << error_message_bytes.data();
    throw std::runtime_error{std::move(ss).str()};
}

void osc::gl::link_program(gl::Program& program)
{
    glLinkProgram(program.get());

    // check for link errors
    GLint link_status = GL_FALSE;
    glGetProgramiv(program.get(), GL_LINK_STATUS, &link_status);

    if (link_status == GL_TRUE) {
        return;
    }

    // else: there were link errors
    GLint log_length = 0;
    glGetProgramiv(program.get(), GL_INFO_LOG_LENGTH, &log_length);

    std::vector<GLchar> error_message_bytes(log_length);
    glGetProgramInfoLog(program.get(), static_cast<GLsizei>(error_message_bytes.size()), nullptr, error_message_bytes.data());

    std::stringstream ss;
    ss << "OpenGL: glLinkProgram() failed: " << error_message_bytes.data();
    throw std::runtime_error{ss.str()};
}

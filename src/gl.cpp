#include "gl.hpp"

#include <sstream>
#include <vector>

using std::literals::operator""s;

gl::Shader_handle::Shader_handle(GLenum shaderType) :
    handle{glCreateShader(shaderType)} {

    if (handle == 0) {
        throw std::runtime_error{"glCreateShader() failed"};
    }
}

gl::Shader_handle gl::CreateShader(GLenum shaderType) {
    return Shader_handle{shaderType};
}

void gl::CompileShader(Shader_handle& sh) {
    glCompileShader(sh.handle);

    // check for compile errors
    GLint params = GL_FALSE;
    glGetShaderiv(sh.handle, GL_COMPILE_STATUS, &params);

    if (params == GL_TRUE) {
        return;
    }

    // else: there were compile errors

    GLint log_len = 0;
    glGetShaderiv(sh.handle, GL_INFO_LOG_LENGTH, &log_len);

    std::vector<GLchar> errmsg(log_len);
    glGetShaderInfoLog(sh.handle, log_len, &log_len, errmsg.data());

    std::stringstream ss;
    ss << errmsg.data();
    throw std::runtime_error{"gl::CompileShader failed: "s + ss.str()};
}

gl::Program gl::CreateProgram() {
    GLuint handle = glCreateProgram();
    if (handle == 0) {
        throw std::runtime_error{"gl::CreateProgram(): failed"};
    }
    return Program{handle};
}

void gl::LinkProgram(gl::Program& prog) {
    glLinkProgram(prog.handle);

    // check for link errors
    GLint link_status = GL_FALSE;
    glGetProgramiv(prog.handle, GL_LINK_STATUS, &link_status);

    if (link_status == GL_TRUE) {
        return;
    }

    // else: there were link errors
    GLint log_len = 0;
    glGetProgramiv(prog.handle, GL_INFO_LOG_LENGTH, &log_len);

    std::vector<GLchar> errmsg(log_len);
    glGetProgramInfoLog(prog.handle, static_cast<GLsizei>(errmsg.size()), nullptr, errmsg.data());

    std::stringstream ss;
    ss << "OpenGL: glLinkProgram() failed: ";
    ss << errmsg.data();
    throw std::runtime_error{ss.str()};
}

GLint gl::GetUniformLocation(Program& p, GLchar const* name) {
    GLint handle = glGetUniformLocation(p.handle, name);
    if (handle == -1) {
        throw std::runtime_error{"glGetUniformLocation() failed: cannot get "s + name};
    }
    return handle;
}

gl::Attribute gl::GetAttribLocation(Program& p, char const* name) {
    GLint handle = glGetAttribLocation(p.handle, name);
    if (handle == -1) {
        throw std::runtime_error{"glGetAttribLocation() failed: cannot get "s + name};
    }
    return Attribute{static_cast<GLuint>(handle)};
}

#pragma once

#include "gl.hpp"

// glm
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>


// gl extensions: useful extension/helper methods over base OpenGL API
//
// these are helpful sugar methods over the base OpenGL API. Anything that is
// OpenGL-ey, but not "pure" OpenGL, goes here.

namespace gl {
    // type-safe wrapper over GL_VERTEX_SHADER
    struct Vertex_shader final {
        Shader_handle handle = CreateShader(GL_VERTEX_SHADER);
    };


    inline void AttachShader(Program& p, Vertex_shader& vs) {
        AttachShader(p, vs.handle);
    }

    // type-safe wrapper over GL_FRAGMENT_SHADER
    struct Fragment_shader final {
        Shader_handle handle = CreateShader(GL_FRAGMENT_SHADER);
    };

    inline void AttachShader(Program& p, Fragment_shader& fs) {
        AttachShader(p, fs.handle);
    }

    // type-safe wrapper over GL_GEOMETRY_SHADER
    struct Geometry_shader final {
        Shader_handle handle = CreateShader(GL_GEOMETRY_SHADER);
    };

    inline void AttachShader(Program& p, Geometry_shader& gs) {
        AttachShader(p, gs.handle);
    }

    void AttachShader(Program& p, Geometry_shader& gs);

    Vertex_shader CompileVertexShader(char const* src);
    Vertex_shader CompileVertexShaderFile(char const* path);
    Fragment_shader CompileFragmentShader(char const* src);
    Fragment_shader CompileFragmentShaderFile(char const* path);
    Geometry_shader CompileGeometryShader(char const* src);
    Geometry_shader CompileGeometryShaderFile(char const* path);

    Program CreateProgramFrom(Vertex_shader const& vs,
                              Fragment_shader const& fs);

    Program CreateProgramFrom(Vertex_shader const& vs,
                              Fragment_shader const& fs,
                              Geometry_shader const& gs);

    // asserts there are no current OpenGL errors (globally)
    void assert_no_errors(char const* func);

    gl::Texture_2d mipmapped_texture(char const* path);

    // set uniforms directly from GLM types
    inline void Uniform(UniformMatrix3fv& u, glm::mat3 const& mat) {
        glUniformMatrix3fv(u.handle, 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(UniformVec4f& u, glm::vec4 const& v) {
        glUniform4fv(u.handle, 1, glm::value_ptr(v));
    }

    inline void Uniform(UniformVec3f& u, glm::vec3 const& v) {
        glUniform3fv(u.handle, 1, glm::value_ptr(v));
    }

    inline void Uniform(UniformMatrix4fv& u, glm::mat4 const& mat) {
        glUniformMatrix4fv(u.handle, 1, false, glm::value_ptr(mat));
    }
}

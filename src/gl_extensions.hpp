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
    // thin wrapper for GL_ARRAY_BUFFER
    class Array_buffer final : public Buffer_handle {
        friend Array_buffer GenArrayBuffer();
        using Buffer_handle::Buffer_handle;
    public:
        static constexpr GLenum type = GL_ARRAY_BUFFER;
    };

    // typed GL_ARRAY_BUFFER equivalent to GenBuffers
    inline Array_buffer GenArrayBuffer() {
        return Array_buffer{};
    }

    // thin wrapper for GL_ELEMENT_ARRAY_BUFFER
    class Element_array_buffer final : public Buffer_handle {
        friend Element_array_buffer GenElementArrayBuffer();
        using Buffer_handle::Buffer_handle;
    public:
        static constexpr GLenum type = GL_ELEMENT_ARRAY_BUFFER;
    };

    // typed GL_ELEMENT_ARRAY_BUFFER equivalent to GenBuffers
    inline Element_array_buffer GenElementArrayBuffer() {
        return Element_array_buffer{};
    }

    // thin wrapper for GL_VERTEX_SHADER
    class Vertex_shader final : public Shader_handle {
        friend Vertex_shader CreateVertexShader();

        Vertex_shader() : Shader_handle{GL_VERTEX_SHADER} {
        }
    };

    // typed GL_VERTEX_SHADER equivalent to CreateShader
    inline Vertex_shader CreateVertexShader() {
        return Vertex_shader{};
    }

    // thin wrapper for GL_FRAGMENT_SHADER
    struct Fragment_shader final : public Shader_handle {
        friend Fragment_shader CreateFragmentShader();

        Fragment_shader() : Shader_handle{GL_FRAGMENT_SHADER} {
        }
    };

    // typed GL_FRAGMENT_SHADER equivalent to CreateShader
    inline Fragment_shader CreateFragmentShader() {
        return Fragment_shader{};
    }

    // thin wrapper for GL_GEOMETRY_SHADER
    class Geometry_shader final : public Shader_handle {
        friend Geometry_shader CreateGeometryShader();

        Geometry_shader() : Shader_handle{GL_GEOMETRY_SHADER} {
        }
    };

    // typed GL_GEOMETRY_SHADER equivalent to CreateShader
    inline Geometry_shader CreateGeometryShader() {
        return Geometry_shader{};
    }

    // type-safe wrapper around GL_TEXTURE_2D
    class Texture_2d final : public Texture_handle {
        friend Texture_2d GenTexture2d();
        using Texture_handle::Texture_handle;
    public:
        static constexpr GLenum type = GL_TEXTURE_2D;

        // HACK: these should be overloaded properly
        operator GLuint () const noexcept {
            return handle;
        }
    };

    // typed GL_TEXTURE_2D equivalent to GenTexture
    inline Texture_2d GenTexture2d() {
        return Texture_2d{};
    }


    // UNIFORMS:

    // type-safe wrapper for glUniform1f
    struct Uniform_1f final {
        GLint handle;
        Uniform_1f(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for glUniform1i
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_1i final {
        GLint handle;
        Uniform_1i(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for glUniformMatrix4fv
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_mat4f final {
        GLint handle;
        Uniform_mat4f(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for glUniformMatrix3fv
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_mat3f final {
        GLint handle;
        Uniform_mat3f(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for UniformVec4f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_vec4f final {
        GLint handle;
        Uniform_vec4f(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for UniformVec3f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_vec3f final {
        GLint handle;
        Uniform_vec3f(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(Uniform_1f& u, GLfloat value) {
        glUniform1f(u.handle, value);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(Uniform_mat4f& u, GLfloat const* value) {
        glUniformMatrix4fv(u.handle, 1, false, value);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(Uniform_1i& u, GLint value) {
        glUniform1i(u.handle, value);
    }

    // set uniforms directly from GLM types
    inline void Uniform(Uniform_mat3f& u, glm::mat3 const& mat) {
        glUniformMatrix3fv(u.handle, 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_vec4f& u, glm::vec4 const& v) {
        glUniform4fv(u.handle, 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec3f& u, glm::vec3 const& v) {
        glUniform3fv(u.handle, 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_mat4f& u, glm::mat4 const& mat) {
        glUniformMatrix4fv(u.handle, 1, false, glm::value_ptr(mat));
    }


    // COMPILE + LINK PROGRAMS:

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


    // OTHER:

    // asserts there are no current OpenGL errors (globally)
    void assert_no_errors(char const* func);

    // read an image file into an OpenGL 2D texture
    gl::Texture_2d mipmapped_texture(char const* path);


}

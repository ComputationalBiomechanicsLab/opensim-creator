#pragma once

#include "gl.hpp"

// glm
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>


// gl extensions: useful extension/helper methods over base OpenGL API
//
// these are helpful sugar methods over the base OpenGL API. Anything that is
// OpenGL-ey, but not "pure" OpenGL, goes here.

// MACROS: usually for sanity-checking during debugging

#define AKGL_STRINGIZE(x) AKGL_STRINGIZE2(x)
#define AKGL_STRINGIZE2(x) #x

#ifndef NDEBUG
#define AKGL_ENABLE(capability) { \
    glEnable(capability); \
    gl::assert_no_errors(__FILE__ ":" AKGL_STRINGIZE(__LINE__) ": glEnable: " #capability); \
}
#else
#define AKGL_ENABLE(capability) { \
    glEnable(capability); \
}
#endif

#define AKGL_ASSERT_NO_ERRORS() { \
    gl::assert_no_errors(__FILE__ ":" AKGL_STRINGIZE(__LINE__)); \
}

namespace gl {
    [[nodiscard]] constexpr Attribute AttributeAtLocation(GLuint loc) noexcept {
        return Attribute{loc};
    }

    // thin wrapper for GL_ARRAY_BUFFER
    class Array_buffer final : public Buffer_handle {
        friend Array_buffer GenArrayBuffer();

        Array_buffer() : Buffer_handle{} {}
    public:
        static constexpr GLenum type = GL_ARRAY_BUFFER;
    };

    // typed GL_ARRAY_BUFFER equivalent to GenBuffers
    inline Array_buffer GenArrayBuffer() {
        return Array_buffer{};
    }

    inline void BindBuffer(Array_buffer& buffer) {
        BindBuffer(buffer.type, buffer);
    }

    // thin wrapper for GL_ELEMENT_ARRAY_BUFFER
    class Element_array_buffer final : public Buffer_handle {
        friend Element_array_buffer GenElementArrayBuffer();

        Element_array_buffer() : Buffer_handle{} {}
    public:
        static constexpr GLenum type = GL_ELEMENT_ARRAY_BUFFER;
    };

    // typed GL_ELEMENT_ARRAY_BUFFER equivalent to GenBuffers
    inline Element_array_buffer GenElementArrayBuffer() {
        return Element_array_buffer{};
    }

    inline void BindBuffer(Element_array_buffer& buffer) {
        BindBuffer(buffer.type, buffer);
    }

    // thin wrapper for GL_VERTEX_SHADER
    class Vertex_shader final : public Shader_handle {
        friend Vertex_shader CreateVertexShader();

        Vertex_shader() : Shader_handle{GL_VERTEX_SHADER} {}
    };

    // typed GL_VERTEX_SHADER equivalent to CreateShader
    inline Vertex_shader CreateVertexShader() {
        return Vertex_shader{};
    }

    // thin wrapper for GL_FRAGMENT_SHADER
    struct Fragment_shader final : public Shader_handle {
        friend Fragment_shader CreateFragmentShader();

        Fragment_shader() : Shader_handle{GL_FRAGMENT_SHADER} {}
    };

    // typed GL_FRAGMENT_SHADER equivalent to CreateShader
    inline Fragment_shader CreateFragmentShader() {
        return Fragment_shader{};
    }

    // thin wrapper for GL_GEOMETRY_SHADER
    class Geometry_shader final : public Shader_handle {
        friend Geometry_shader CreateGeometryShader();

        Geometry_shader() : Shader_handle{GL_GEOMETRY_SHADER} {}
    };

    // typed GL_GEOMETRY_SHADER equivalent to CreateShader
    inline Geometry_shader CreateGeometryShader() {
        return Geometry_shader{};
    }

    // type-safe wrapper around GL_TEXTURE_2D
    class Texture_2d final : public Texture_handle {
        friend Texture_2d GenTexture2d();

        Texture_2d() : Texture_handle{} {}
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

    // convenience wrapper for glBindTexture
    inline void BindTexture(Texture_2d const& texture) {
        BindTexture(texture.type, texture);
    }

    // type-safe wrapper around GL_TEXTURE_CUBE_MAP
    class Texture_cubemap final : public Texture_handle {
        friend Texture_cubemap GenTextureCubemap();

        Texture_cubemap() : Texture_handle{} {}
    public:
        static constexpr GLenum type = GL_TEXTURE_CUBE_MAP;

        // HACK: these should be overloaded properly
        operator GLuint () const noexcept {
            return handle;
        }
    };

    // typed GL_TEXTURE_CUBE_MAP equivalent to GenTexture
    inline Texture_cubemap GenTextureCubemap() {
        return Texture_cubemap{};
    }

    inline void BindTexture(Texture_cubemap const& texture) {
        BindTexture(texture.type, texture);
    }

    class Texture_2d_multisample final : public Texture_handle {
        friend Texture_2d_multisample GenTexture2dMultisample();

        Texture_2d_multisample() : Texture_handle{} {}
    public:
        static constexpr GLenum type = GL_TEXTURE_2D_MULTISAMPLE;

        // HACK: these should be overloaded properly
        operator GLuint () const noexcept {
            return handle;
        }
    };

    inline Texture_2d_multisample GenTexture2dMultisample() {
        return Texture_2d_multisample{};
    }

    inline void BindTexture(Texture_2d_multisample const& texture) {
        BindTexture(texture.type, texture);
    }


    // UNIFORMS:

    // type-safe wrapper for glUniform1f
    struct Uniform_float final {
        GLint handle;
        Uniform_float(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for glUniform1i
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_int final {
        GLint handle;
        Uniform_int(GLint _handle) : handle{_handle} {}
    };

    using Uniform_sampler2d = Uniform_int;
    using Uniform_samplerCube = Uniform_int;

    // type-safe wrapper for glUniformMatrix4fv
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_mat4 final {
        GLint handle;
        Uniform_mat4(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for glUniformMatrix3fv
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_mat3 final {
        GLint handle;
        Uniform_mat3(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for UniformVec4f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_vec4 final {
        GLint handle;
        Uniform_vec4(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for UniformVec3f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform_vec3 final {
        GLint handle;
        Uniform_vec3(GLint _handle) : handle{_handle} {}
    };

    struct Uniform_vec2f final {
        GLint handle;
        Uniform_vec2f(GLint _handle) : handle{_handle} {}
    };

    using Uniform_bool = Uniform_int;

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(Uniform_float& u, GLfloat value) {
        glUniform1f(u.handle, value);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(Uniform_mat4& u, GLfloat const* value) {
        glUniformMatrix4fv(u.handle, 1, false, value);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(Uniform_int& u, GLint value) {
        glUniform1i(u.handle, value);
    }

    inline void Uniform(Uniform_int& u, GLsizei n, GLint const* vs) {
        glUniform1iv(u.handle, n, vs);
    }

    // set uniforms directly from GLM types
    inline void Uniform(Uniform_mat3& u, glm::mat3 const& mat) {
        glUniformMatrix3fv(u.handle, 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_vec4& u, glm::vec4 const& v) {
        glUniform4fv(u.handle, 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec3& u, glm::vec3 const& v) {
        glUniform3fv(u.handle, 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec3& u, float x, float y, float z) {
        glUniform3f(u.handle, x, y, z);
    }

    inline void Uniform(Uniform_mat4& u, glm::mat4 const& mat) {
        glUniformMatrix4fv(u.handle, 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_mat4& u, GLsizei n, glm::mat4 const* first) {
        static_assert(sizeof(glm::mat4) == 16*sizeof(GLfloat));
        glUniformMatrix4fv(u.handle, n, false, glm::value_ptr(*first));
    }

    struct Uniform_identity_val_tag {};

    inline Uniform_identity_val_tag identity_val;

    inline void Uniform(Uniform_mat4& u, Uniform_identity_val_tag) {
        Uniform(u, glm::identity<glm::mat4>());
    }

    inline void Uniform(Uniform_vec2f& u, glm::vec2 const& v) {
        glUniform2fv(u.handle, 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec2f& u, GLsizei n, glm::vec2 const* vs) {
        // sanity assert: GLM vecs *should* only contain tightly packed floats
        // if they don't, then the OpenGL Uniform2fv call will fail.
        static_assert(sizeof(glm::vec2) == 2*sizeof(float));

        glUniform2fv(u.handle, n, glm::value_ptr(*vs));
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
    void assert_no_errors(char const* label);

    // read an image file into an OpenGL 2D texture
    gl::Texture_2d flipped_and_mipmapped_texture(char const* path, bool srgb = false);

    gl::Texture_2d nonflipped_and_mipmapped_texture(char const* path);

    // read 6 image files into a single OpenGL cubemap (GL_TEXTURE_CUBE_MAP)
    gl::Texture_cubemap read_cubemap(
            char const* path_pos_x,
            char const* path_neg_x,
            char const* path_pos_y,
            char const* path_neg_y,
            char const* path_pos_z,
            char const* path_neg_z);

    inline glm::mat3 normal_matrix(glm::mat4 const& m) {
         return glm::transpose(glm::inverse(m));
    }

    template<GLenum E>
    inline constexpr unsigned texture_index() {
        static_assert(GL_TEXTURE0 <= E and E <= GL_TEXTURE30);
        return E - GL_TEXTURE0;
    }

    template<typename T>
    class Sized_array_buffer final {
        size_t _size = 0;
        gl::Array_buffer _vbo = gl::GenArrayBuffer();

    public:     
        template<typename Collection>
        Sized_array_buffer(Collection const& c) :
            Sized_array_buffer{c.begin(), c.end()} {
        }

        Sized_array_buffer(std::initializer_list<T> const& els) :
            Sized_array_buffer{els.begin(), els.end()} {
        }

        Sized_array_buffer(T const* begin, T const* end) :
            _size{static_cast<size_t>(std::distance(begin, end))} {

            gl::BindBuffer(_vbo);
            gl::BufferData(_vbo.type, static_cast<long>(_size * sizeof(T)), begin, GL_STATIC_DRAW);
        }

        gl::Array_buffer& data() noexcept {
            return _vbo;
        }

        gl::Array_buffer const& data() const noexcept {
            return _vbo;
        }

        size_t size() const noexcept {
            return _size;
        }

        GLsizei sizei() const noexcept {
            return static_cast<GLsizei>(_size);
        }
    };
}

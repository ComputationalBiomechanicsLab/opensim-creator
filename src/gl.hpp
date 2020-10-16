#pragma once

#include <GL/glew.h>

// gl: thin C++ wrappers around OpenGL
//
// Code in here should:
//
//   - Roughly map 1:1 with OpenGL
//   - Add RAII to types that have destruction methods
//     (e.g. `glDeleteShader`)
//   - Use exceptions to enforce basic invariants (e.g. compiling a shader
//     should work, or throw)
//
// Emphasis is on simplicity, not "abstraction correctness". It is preferred
// to have an API that is simple, rather than robustly encapsulated etc.
//
// `inline`ing here is alright, provided it just forwards the gl calls and
//  doesn't do much else (e.g. throw exceptions, which this header doesn't
//  expose to keep compilation units small)

namespace gl {
    // RAII wrapper for glDeleteShader
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteShader.xhtml
    struct Shader_handle final {
        GLuint handle;

    private:
        friend Shader_handle CreateShader(GLenum);
        Shader_handle(GLuint _handle) : handle{_handle} {
        }
    public:
        Shader_handle(Shader_handle const&) = delete;
        Shader_handle(Shader_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = 0;
        }
        Shader_handle& operator=(Shader_handle const&) = delete;
        Shader_handle& operator=(Shader_handle&&) = delete;
        ~Shader_handle() noexcept {
            if (handle != 0) {
                glDeleteShader(handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCreateShader.xhtml
    //     *throws on error
    Shader_handle CreateShader(GLenum shaderType);

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glShaderSource.xhtml
    inline void ShaderSource(Shader_handle& sh, char const* src) {
        glShaderSource(sh.handle, 1, &src, nullptr);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCompileShader.xhtml
    //     *throws on error
    void CompileShader(Shader_handle& sh);

    // RAII for glDeleteProgram
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteProgram.xhtml
    struct Program final {
        GLuint handle;

    private:
        friend Program CreateProgram();
        Program(GLuint _handle) : handle{_handle} {
        }
    public:
        Program(Program const&) = delete;
        Program(Program&& tmp) : handle{tmp.handle} {
            tmp.handle = 0;
        }
        Program& operator=(Program const&) = delete;
        Program& operator=(Program&&) = delete;
        ~Program() noexcept {
            if (handle != 0) {
                glDeleteProgram(handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCreateProgram.xhtml
    //     *throws on error
    Program CreateProgram();

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram(Program& p) {
        glUseProgram(p.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram() {
        glUseProgram(static_cast<GLuint>(0));
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glAttachShader.xhtml
    inline void AttachShader(Program& p, Shader_handle const& sh) {
        glAttachShader(p.handle, sh.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glLinkProgram.xhtml
    void LinkProgram(gl::Program& prog);

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    //     *throws on error
    GLint GetUniformLocation(Program& p, GLchar const* name);

    // type-safe wrapper for glUniform1f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform1f final {
        GLint handle;
        Uniform1f(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(Uniform1f& u, GLfloat value) {
        glUniform1f(u.handle, value);
    }

    // type-safe wrapper for glUniform1i
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform1i final {
        GLint handle;
        Uniform1i(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(Uniform1i& u, GLint value) {
        glUniform1i(u.handle, value);
    }

    // type-safe wrapper for glUniformMatrix4fv
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct UniformMatrix4fv final {
        GLint handle;
        UniformMatrix4fv(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    inline void Uniform(UniformMatrix4fv& u, GLfloat const* value) {
        glUniformMatrix4fv(u.handle, 1, false, value);
    }

    // type-safe wrapper for glUniformMatrix3fv
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct UniformMatrix3fv final {
        GLint handle;
        UniformMatrix3fv(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for UniformVec4f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct UniformVec4f final {
        GLint handle;
        UniformVec4f(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for UniformVec3f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct UniformVec3f final {
        GLint handle;
        UniformVec3f(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for an GLSL attribute index
    //     (just prevents accidently handing a GLint to the wrong API)
    struct Attribute final {
        GLint handle;
        constexpr Attribute(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetAttribLocation.xhtml
    //     *throws on error
    Attribute GetAttribLocation(Program& p, char const* name);

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
    inline void VertexAttribPointer(Attribute const& a,
                                    GLint size,
                                    GLenum type,
                                    GLboolean normalized,
                                    GLsizei stride,
                                    const void * pointer) {
        glVertexAttribPointer(a.handle,
                              size,
                              type,
                              normalized,
                              stride,
                              pointer);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glEnableVertexAttribArray.xhtml
    inline void EnableVertexAttribArray(Attribute const& a) {
        glEnableVertexAttribArray(a.handle);
    }

    // RAII wrapper for glDeleteBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteBuffers.xhtml
    struct Buffer_handle final {
        GLuint handle;

    private:
        friend Buffer_handle GenBuffers();
        Buffer_handle(GLuint _handle) : handle{_handle} {
        }
    public:
        Buffer_handle(Buffer_handle const&) = delete;
        Buffer_handle(Buffer_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Buffer_handle& operator=(Buffer_handle const&) = delete;
        Buffer_handle& operator=(Buffer_handle&&) = delete;
        ~Buffer_handle() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteBuffers(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenBuffers.xhtml
    inline Buffer_handle GenBuffers() {
        GLuint handle;
        glGenBuffers(1, &handle);
        return Buffer_handle{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    inline void BindBuffer(GLenum target, Buffer_handle& buffer) {
        glBindBuffer(target, buffer.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
    inline void BufferData(GLenum target,
                           size_t num_bytes,
                           void const* data,
                           GLenum usage) {
        glBufferData(target, num_bytes, data, usage);
    }

    // type-safe wrapper for GL_ARRAY_BUFFER
    struct Array_buffer final {
        Buffer_handle handle = GenBuffers();
    };

    // type-safe sugar over glBindBuffer
    inline void BindBuffer(Array_buffer& buffer) {
        BindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    }

    // type-safe sugar over glBufferData
    inline void BufferData(Array_buffer&,
                           size_t num_bytes,
                           void const* data,
                           GLenum usage) {
        glBufferData(GL_ARRAY_BUFFER, num_bytes, data, usage);
    }

    // type-safe wrapper for GL_ELEMENT_ARRAY_BUFFER
    struct Element_array_buffer final {
        Buffer_handle handle = GenBuffers();
    };

    // type-safe sugar over glBindBuffer
    inline void BindBuffer(Element_array_buffer& buffer) {
        BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.handle);
    }

    // type-safe sugar over glBufferData
    inline void BufferData(Element_array_buffer&,
                           size_t num_bytes,
                           void const* data,
                           GLenum usage) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_bytes, data, usage);
    }

    // RAII wrapper for glDeleteVertexArrays
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteVertexArrays.xhtml
    struct Vertex_array final {
        GLuint handle;

    private:
        friend Vertex_array GenVertexArrays();
        Vertex_array(GLuint _handle) : handle{_handle} {
        }
    public:
        Vertex_array(Vertex_array const&) = delete;
        Vertex_array(Vertex_array&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Vertex_array& operator=(Vertex_array const&) = delete;
        Vertex_array& operator=(Vertex_array&&) = delete;
        ~Vertex_array() noexcept {
            if (handle == static_cast<GLuint>(-1)) {
                glDeleteVertexArrays(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenVertexArrays.xhtml
    Vertex_array GenVertexArrays();

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray(Vertex_array& vao) {
        glBindVertexArray(vao.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml    
    inline void BindVertexArray() {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    // RAII wrapper for glDeleteTextures
    //     https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glDeleteTextures.xml
    struct Texture_handle final {
        GLuint handle;

    private:
        friend Texture_handle GenTextures();
        Texture_handle(GLuint _handle) : handle{_handle} {
        }
    public:
        Texture_handle(Texture_handle const&) = delete;
        Texture_handle(Texture_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Texture_handle& operator=(Texture_handle const&) = delete;
        Texture_handle& operator=(Texture_handle&&) = delete;
        ~Texture_handle() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteTextures(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenTextures.xhtml
    Texture_handle GenTextures();

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture(GLenum target, Texture_handle& texture) {
        glBindTexture(target, texture.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // type-safe wrapper around GL_TEXTURE_2D
    struct Texture_2d final {
        Texture_handle handle = GenTextures();

        // HACK: these should be overloaded properly
        operator GLuint () const noexcept {
            return handle.handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture(Texture_2d& t) {
        BindTexture(GL_TEXTURE_2D, t.handle);
    }

    // RAII wrapper for glDeleteFrameBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteFramebuffers.xhtml
    struct Frame_buffer final {
        GLuint handle;

    private:
        friend Frame_buffer GenFrameBuffer();
        Frame_buffer(GLuint _handle) : handle{_handle} {
        }
    public:
        Frame_buffer(Frame_buffer const&) = delete;
        Frame_buffer(Frame_buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Frame_buffer& operator=(Frame_buffer const&) = delete;
        Frame_buffer& operator=(Frame_buffer&&) = delete;
        ~Frame_buffer() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteFramebuffers(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenFramebuffers.xhtml
    Frame_buffer GenFrameBuffer();

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    void BindFrameBuffer(GLenum target, Frame_buffer const& fb);

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    void BindFrameBuffer();

    // RAII wrapper for glDeleteRenderBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteRenderbuffers.xhtml
    struct Render_buffer final {
        GLuint handle;

    private:
        friend Render_buffer GenRenderBuffer();
        Render_buffer(GLuint _handle) : handle{_handle} {
        }
    public:
        Render_buffer(Render_buffer const&) = delete;
        Render_buffer(Render_buffer&&) = delete;
        Render_buffer& operator=(Render_buffer const&) = delete;
        Render_buffer& operator=(Render_buffer&&) = delete;
        ~Render_buffer() noexcept {
            glDeleteRenderbuffers(1, &handle);
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glGenRenderbuffers.xml
    Render_buffer GenRenderBuffer();

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    void BindRenderBuffer(Render_buffer& rb);

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    void BindRenderBuffer();
}

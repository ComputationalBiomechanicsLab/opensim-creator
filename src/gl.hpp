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
//  doesn't do much else

namespace gl {
    // RAII wrapper for glDeleteShader
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteShader.xhtml
    struct Shader_handle {
        friend Shader_handle CreateShader(GLenum);

        GLuint handle;

    protected:
        Shader_handle(GLenum shaderType);
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
    void LinkProgram(Program& prog);

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    //     *throws on error
    GLint GetUniformLocation(Program& p, GLchar const* name);

    // type-safe wrapper for an GLSL attribute index
    //     (just prevents accidently handing a GLint to the wrong API)
    struct Attribute {
        GLuint handle;
        explicit constexpr Attribute(GLuint _handle) : handle{_handle} {}
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
    struct Buffer_handle {
        friend Buffer_handle GenBuffers();

        GLuint handle;

    protected:
        Buffer_handle() {
            glGenBuffers(1, &handle);
        }
    public:
        Buffer_handle(Buffer_handle const&) = delete;
        Buffer_handle(Buffer_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Buffer_handle& operator=(Buffer_handle const&) = delete;
        Buffer_handle& operator=(Buffer_handle&& tmp) {
            GLuint h = handle;
            handle = tmp.handle;
            tmp.handle = h;
            return *this;
        }
        ~Buffer_handle() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteBuffers(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenBuffers.xhtml
    inline Buffer_handle GenBuffers() {
        return Buffer_handle{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    inline void BindBuffer(GLenum target, Buffer_handle& buffer) {
        glBindBuffer(target, buffer.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    //     *overload that unbinds current buffer
    inline void BindBuffer() {
        // from docs:
        // > Instead, buffer set to zero effectively unbinds any buffer object
        // > previously bound, and restores client memory usage for that buffer
        // > object target (if supported for that target)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
    inline void BufferData(GLenum target,
                           GLsizeiptr num_bytes,
                           void const* data,
                           GLenum usage) {
        glBufferData(target, num_bytes, data, usage);
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
        Vertex_array& operator=(Vertex_array&& tmp) {
            GLuint h = handle;
            handle = tmp.handle;
            tmp.handle = h;
            return *this;
        }
        ~Vertex_array() noexcept {
            if (handle == static_cast<GLuint>(-1)) {
                glDeleteVertexArrays(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenVertexArrays.xhtml
    inline Vertex_array GenVertexArrays() {
        GLuint handle;
        glGenVertexArrays(1, &handle);
        return Vertex_array{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray(Vertex_array& vao) {
        glBindVertexArray(vao.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray() {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    // RAII wrapper for glGenTextures/glDeleteTextures
    //     https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glDeleteTextures.xml
    struct Texture_handle {
        friend Texture_handle GenTextures();

        GLuint handle;

    protected:
        Texture_handle() {
            glGenTextures(1, &handle);
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
    inline Texture_handle GenTextures() {
        return Texture_handle{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glActiveTexture.xhtml
    inline void ActiveTexture(GLenum texture) {
        glActiveTexture(texture);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture(GLenum target, Texture_handle const& texture) {
        glBindTexture(target, texture.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    inline void TexImage2D(
            GLenum target,
            GLint level,
            GLint internalformat,
            GLsizei width,
            GLsizei height,
            GLint border,
            GLenum format,
            GLenum type,
            const void * data) {
        glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
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
    inline Frame_buffer GenFrameBuffer() {
        GLuint handle;
        glGenFramebuffers(1, &handle);
        return Frame_buffer{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFrameBuffer(GLenum target, GLuint handle) {
        glBindFramebuffer(target, handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFrameBuffer(GLenum target, Frame_buffer const& fb) {
        glBindFramebuffer(target, fb.handle);
    }

    static constexpr GLuint window_fbo = 0;

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glFramebufferTexture2D.xml
    inline void FramebufferTexture2D(
            GLenum target,
            GLenum attachment,
            GLenum textarget,
            GLuint texture,
            GLint level) {
        glFramebufferTexture2D(target, attachment, textarget, texture, level);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBlitFramebuffer.xhtml
    inline void BlitFramebuffer(
            GLint srcX0,
            GLint srcY0,
            GLint srcX1,
            GLint srcY1,
            GLint dstX0,
            GLint dstY0,
            GLint dstX1,
            GLint dstY1,
            GLbitfield mask,
            GLenum filter) {
        glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    }

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
    inline Render_buffer GenRenderBuffer() {
        GLuint handle;
        glGenRenderbuffers(1, &handle);
        return Render_buffer{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer(Render_buffer& rb) {
        glBindRenderbuffer(GL_RENDERBUFFER, rb.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer() {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glClear.xhtml
    inline void Clear(GLbitfield mask) {
        glClear(mask);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArrays.xhtml
    inline void DrawArrays(GLenum mode, GLint first, GLsizei count) {
        glDrawArrays(mode, first, count);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArraysInstanced.xhtml
    inline void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
        glDrawArraysInstanced(mode, first, count, instancecount);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribDivisor.xhtml
    inline void VertexAttribDivisor(gl::Attribute const& attr, GLuint divisor) {
        glVertexAttribDivisor(attr.handle, divisor);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawElements.xhtml
    inline void DrawElements(GLenum mode, GLsizei count, GLenum type, const void * indices) {
        glDrawElements(mode, count, type, indices);
    }

    inline void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
        glClearColor(red, green, blue, alpha);
    }

    inline void Viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
        glViewport(x, y, w, h);
    }
}

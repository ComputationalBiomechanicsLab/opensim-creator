#pragma once

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cassert>
#include <filesystem>
#include <stdexcept>

namespace gl {
    std::string slurp(std::filesystem::path const& path);

    // RAII wrapper for glDeleteShader
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteShader.xhtml
    class Shader {
        GLuint handle;

    public:
        static constexpr GLuint empty_handle = 0;

        Shader(GLenum type) : handle{glCreateShader(type)} {
            if (handle == empty_handle) {
                throw std::runtime_error{"glCreateShader: returned an empty handle"};
            }
        }
        Shader(Shader const&) = delete;
        Shader(Shader&& tmp) : handle{tmp.handle} {
            tmp.handle = empty_handle;
        }
        Shader& operator=(Shader const&) = delete;
        Shader& operator=(Shader&&) = delete;
        ~Shader() noexcept {
            if (handle != empty_handle) {
                glDeleteShader(handle);
            }
        }

        operator GLuint() noexcept {
            return handle;
        }

        operator GLuint() const noexcept {
            return handle;  // TODO: this should be removed but is handy in (e.g. CreateProgramFrom)
        }
    };

    struct Vertex_shader final : public Shader {
        Vertex_shader() : Shader{GL_VERTEX_SHADER} {
        }
    };

    struct Fragment_shader final : public Shader {
        Fragment_shader() : Shader{GL_FRAGMENT_SHADER} {
        }
    };

    struct Geometry_shader final : public Shader {
        Geometry_shader() : Shader{GL_GEOMETRY_SHADER} {
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glShaderSource.xhtml
    inline void ShaderSource(Shader& sh, char const* src) {
        glShaderSource(sh, 1, &src, nullptr);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCompileShader.xhtml
    //     *throws on error
    void CompileShader(Shader& sh);

    template<typename T>
    T Compile(char const* src) {
        T shader;
        ShaderSource(shader, src);
        CompileShader(shader);
        return shader;
    }

    template<typename T>
    T Compile(std::string const& src) {
        return Compile<T>(src.c_str());
    }

    template<typename T>
    T Compile(std::filesystem::path const& path) {
        try {
            std::string src = slurp(path);
            return Compile<T>(src);
        } catch (std::exception const& ex) {
            throw std::runtime_error{path.string() + ": cannot compile shader: " + ex.what()};
        }
    }

    // RAII for glDeleteProgram
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteProgram.xhtml
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCreateProgram.xhtml
    class Program final {
        GLuint handle;

    public:
        static constexpr GLuint empty_handle = 0;

        Program() : handle{glCreateProgram()} {
            if (handle == empty_handle) {
                throw std::runtime_error{"glCreateProgram: returned an empty handle"};
            }
        }
        Program(Program const&) = delete;
        Program(Program&& tmp) : handle{tmp.handle} {
            tmp.handle = empty_handle;
        }
        Program& operator=(Program const&) = delete;
        Program& operator=(Program&&) = delete;
        ~Program() noexcept {
            if (handle != empty_handle) {
                glDeleteProgram(handle);
            }
        }

        operator GLuint() noexcept {
            return handle;
        }
    };

    inline Program CreateProgram() {
        return Program{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram(Program& p) {
        glUseProgram(p);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram() {
        glUseProgram(Program::empty_handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glAttachShader.xhtml
    inline void AttachShader(Program& p, Shader const& sh) {
        glAttachShader(p, sh);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glLinkProgram.xhtml
    //     *throws on error
    void LinkProgram(Program& prog);

    inline Program CreateProgramFrom(Vertex_shader const& vs, Fragment_shader const& fs) {
        gl::Program p;
        AttachShader(p, vs);
        AttachShader(p, fs);
        LinkProgram(p);
        return p;
    }

    inline Program CreateProgramFrom(Vertex_shader const& vs, Fragment_shader const& fs, Geometry_shader const& gs) {
        gl::Program p;
        AttachShader(p, vs);
        AttachShader(p, gs);
        AttachShader(p, fs);
        LinkProgram(p);
        return p;
    }

    // basic wrapper around an attribute
    class Attribute {
        GLuint location;

    public:
        explicit constexpr Attribute(GLuint _handle) : location{_handle} {
        }

        Attribute(Program& p, char const* name) {
            GLint handle = glGetAttribLocation(p, name);
            if (handle == -1) {
                throw std::runtime_error{std::string{"glGetAttribLocation() failed: cannot get "} + name};
            }
            location = static_cast<GLuint>(handle);
        }

        operator GLuint() noexcept {
            return location;
        }

        operator GLuint() const noexcept {
            return location;
        }
    };

    constexpr Attribute AttributeAtLocation(GLuint loc) {
        return Attribute{loc};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
    inline void VertexAttribPointer(
        Attribute const& a, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
        glVertexAttribPointer(a, size, type, normalized, stride, pointer);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glEnableVertexAttribArray.xhtml
    inline void EnableVertexAttribArray(Attribute const& a) {
        glEnableVertexAttribArray(a);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    //     *throws on error
    inline GLint GetUniformLocation(Program& p, GLchar const* name) {
        GLint handle = glGetUniformLocation(p, name);
        if (handle == -1) {
            throw std::runtime_error{std::string{"glGetUniformLocation() failed: cannot get "} + name};
        }
        return handle;
    }

    class Uniform_handle {
        GLint location;

    public:
        Uniform_handle(Program& p, char const* name) : location{GetUniformLocation(p, name)} {
        }
        Uniform_handle(GLint _location) : location{_location} {
        }

        operator GLint() noexcept {
            return location;
        }
    };

    class Uniform_float final : public Uniform_handle {
        using Uniform_handle::Uniform_handle;
    };
    class Uniform_int final : public Uniform_handle {
        using Uniform_handle::Uniform_handle;
    };
    using Uniform_bool = Uniform_int;
    using Uniform_sampler2d = Uniform_int;
    using Uniform_sampler2DMS = Uniform_int;
    using Uniform_samplerCube = Uniform_int;
    class Uniform_mat4 final : public Uniform_handle {
        using Uniform_handle::Uniform_handle;
    };
    class Uniform_mat3 final : public Uniform_handle {
        using Uniform_handle::Uniform_handle;
    };
    class Uniform_vec4 final : public Uniform_handle {
        using Uniform_handle::Uniform_handle;
    };
    class Uniform_vec3 final : public Uniform_handle {
        using Uniform_handle::Uniform_handle;
    };
    class Uniform_vec2 final : public Uniform_handle {
        using Uniform_handle::Uniform_handle;
    };

    struct Uniform_identity_val_tag {};
    inline Uniform_identity_val_tag identity_val;

    inline void Uniform(Uniform_float& u, GLfloat value) {
        glUniform1f(u, value);
    }

    inline void Uniform(Uniform_mat4& u, GLfloat const* value) {
        glUniformMatrix4fv(u, 1, false, value);
    }

    inline void Uniform(Uniform_int& u, GLint value) {
        glUniform1i(u, value);
    }

    inline void Uniform(Uniform_int& u, GLsizei n, GLint const* vs) {
        glUniform1iv(u, n, vs);
    }

    inline void Uniform(Uniform_mat3& u, glm::mat3 const& mat) {
        glUniformMatrix3fv(u, 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_vec4& u, glm::vec4 const& v) {
        glUniform4fv(u, 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec3& u, glm::vec3 const& v) {
        glUniform3fv(u, 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec4& u, float x, float y, float z, float a) {
        Uniform(u, glm::vec4{x, y, z, a});
    }

    inline void Uniform(Uniform_vec3& u, float x, float y, float z) {
        glUniform3f(u, x, y, z);
    }

    inline void Uniform(Uniform_mat4& u, glm::mat4 const& mat) {
        glUniformMatrix4fv(u, 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_mat4& u, GLsizei n, glm::mat4 const* first) {
        static_assert(sizeof(glm::mat4) == 16 * sizeof(GLfloat));
        glUniformMatrix4fv(u, n, false, glm::value_ptr(*first));
    }

    inline void Uniform(Uniform_mat4& u, Uniform_identity_val_tag) {
        Uniform(u, glm::identity<glm::mat4>());
    }

    inline void Uniform(Uniform_vec2& u, glm::vec2 const& v) {
        glUniform2fv(u, 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec2& u, GLsizei n, glm::vec2 const* vs) {
        static_assert(sizeof(glm::vec2) == 2 * sizeof(float));
        glUniform2fv(u, n, glm::value_ptr(*vs));
    }

    // RAII wrapper for glDeleteBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteBuffers.xhtml
    class Buffer {
        GLuint handle;

    public:
        static constexpr GLuint empty_handle = static_cast<GLuint>(-1);

        Buffer() {
            glGenBuffers(1, &handle);
            if (handle == empty_handle) {
                throw std::runtime_error{"glGenBuffers: returned an empty handle"};
            }
        }
        Buffer(Buffer const&) = delete;
        Buffer(Buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = empty_handle;
        }
        Buffer& operator=(Buffer const&) = delete;
        Buffer& operator=(Buffer&& tmp) {
            GLuint h = handle;
            handle = tmp.handle;
            tmp.handle = h;
            return *this;
        }
        ~Buffer() noexcept {
            if (handle != empty_handle) {
                glDeleteBuffers(1, &handle);
            }
        }

        operator GLuint() noexcept {
            return handle;
        }
    };

    struct Array_buffer final : public Buffer {
        static constexpr GLenum type = GL_ARRAY_BUFFER;
    };

    struct Element_array_buffer final : public Buffer {
        static constexpr GLenum type = GL_ELEMENT_ARRAY_BUFFER;
    };

    struct Pixel_pack_buffer final : public Buffer {
        static constexpr GLenum type = GL_PIXEL_PACK_BUFFER;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenBuffers.xhtml
    inline Buffer GenBuffers() {
        return Buffer{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    inline void BindBuffer(GLenum target, Buffer& buffer) {
        glBindBuffer(target, buffer);
    }

    inline void UnbindBuffer(GLenum target) {
        glBindBuffer(target, 0);
    }

    inline void BindBuffer(Array_buffer& buffer) {
        BindBuffer(buffer.type, buffer);
    }

    inline void BindBuffer(Element_array_buffer& buffer) {
        BindBuffer(buffer.type, buffer);
    }

    inline void BindBuffer(Pixel_pack_buffer& buffer) {
        BindBuffer(buffer.type, buffer);
    }

    inline void UnbindBuffer(Pixel_pack_buffer& buffer) {
        UnbindBuffer(buffer.type);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    //     *overload that unbinds current buffer
    inline void BindBuffer() {
        // from docs:
        // > Instead, buffer set to zero effectively unbinds any buffer object
        // > previously bound, and restores client memory usage for that buffer
        // > object target (if supported for that target)
        glBindBuffer(GL_ARRAY_BUFFER, Buffer::empty_handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
    inline void BufferData(GLenum target, GLsizeiptr num_bytes, void const* data, GLenum usage) {
        glBufferData(target, num_bytes, data, usage);
    }

    // RAII wrapper for glDeleteVertexArrays
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteVertexArrays.xhtml
    class Vertex_array final {
        GLuint handle;

    public:
        static constexpr GLuint empty_handle = static_cast<GLuint>(-1);

        Vertex_array() {
            glGenVertexArrays(1, &handle);
            if (handle == empty_handle) {
                throw std::runtime_error{"glGenVertexArrays: returned an empty handle"};
            }
        }
        Vertex_array(Vertex_array const&) = delete;
        Vertex_array(Vertex_array&& tmp) : handle{tmp.handle} {
            tmp.handle = empty_handle;
        }
        Vertex_array& operator=(Vertex_array const&) = delete;
        Vertex_array& operator=(Vertex_array&& tmp) {
            GLuint h = handle;
            handle = tmp.handle;
            tmp.handle = h;
            return *this;
        }
        ~Vertex_array() noexcept {
            if (handle == empty_handle) {
                glDeleteVertexArrays(1, &handle);
            }
        }

        operator GLuint() noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenVertexArrays.xhtml
    inline Vertex_array GenVertexArrays() {
        return Vertex_array{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray(Vertex_array& vao) {
        glBindVertexArray(vao);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray() {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    // RAII wrapper for glGenTextures/glDeleteTextures
    //     https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glDeleteTextures.xml
    class Texture {
        GLuint handle;

    public:
        static constexpr GLuint empty_handle = static_cast<GLuint>(-1);

        Texture() {
            glGenTextures(1, &handle);
            if (handle == empty_handle) {
                throw std::runtime_error{"glGenTextures: returned an empty handle"};
            }
        }
        Texture(Texture const& src) = delete;
        Texture(Texture&& tmp) : handle{tmp.handle} {
            tmp.handle = empty_handle;
        }
        Texture& operator=(Texture const&) = delete;
        Texture& operator=(Texture&& tmp) {
            GLuint h = tmp.handle;
            tmp.handle = handle;
            handle = h;
            return *this;
        }
        ~Texture() noexcept {
            if (handle != empty_handle) {
                glDeleteTextures(1, &handle);
            }
        }

        operator GLuint() noexcept {
            return handle;
        }
    };

    struct Texture_2d final : public Texture {
        static constexpr GLenum type = GL_TEXTURE_2D;
    };

    struct Texture_cubemap final : public Texture {
        static constexpr GLenum type = GL_TEXTURE_CUBE_MAP;
    };

    struct Texture_2d_multisample final : public Texture {
        static constexpr GLenum type = GL_TEXTURE_2D_MULTISAMPLE;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenTextures.xhtml
    inline Texture GenTextures() {
        return Texture{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glActiveTexture.xhtml
    inline void ActiveTexture(GLenum texture) {
        glActiveTexture(texture);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture(GLenum target, Texture& texture) {
        glBindTexture(target, texture);
    }

    inline void BindTexture(Texture_2d& texture) {
        BindTexture(texture.type, texture);
    }

    inline void BindTexture(Texture_cubemap& texture) {
        BindTexture(texture.type, texture);
    }

    inline void BindTexture(Texture_2d_multisample& texture) {
        BindTexture(texture.type, texture);
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
        const void* data) {
        glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
    }

    template<GLenum E>
    inline constexpr unsigned texture_index() {
        static_assert(GL_TEXTURE0 <= E and E <= GL_TEXTURE30);
        return E - GL_TEXTURE0;
    }

    // RAII wrapper for glDeleteFrameBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteFramebuffers.xhtml
    class Frame_buffer final {
        GLuint handle;

    public:
        static constexpr GLuint empty_handle = static_cast<GLuint>(-1);

        Frame_buffer() {
            glGenFramebuffers(1, &handle);
            if (handle == empty_handle) {
                throw std::runtime_error{"glGenFramebuffers: returned empty handle"};
            }
        }

        Frame_buffer(Frame_buffer const&) = delete;
        Frame_buffer(Frame_buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = empty_handle;
        }
        Frame_buffer& operator=(Frame_buffer const&) = delete;
        Frame_buffer& operator=(Frame_buffer&& tmp) {
            GLuint h = tmp.handle;
            tmp.handle = handle;
            handle = h;
            return *this;
        }
        ~Frame_buffer() noexcept {
            if (handle != empty_handle) {
                glDeleteFramebuffers(1, &handle);
            }
        }

        operator GLuint() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenFramebuffers.xhtml
    inline Frame_buffer GenFrameBuffer() {
        return Frame_buffer{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFrameBuffer(GLenum target, GLuint handle) {
        glBindFramebuffer(target, handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFrameBuffer(GLenum target, Frame_buffer const& fb) {
        glBindFramebuffer(target, fb);
    }

    static constexpr GLuint window_fbo = 0;

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glFramebufferTexture2D.xml
    inline void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
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
    class Render_buffer final {
        GLuint handle;

    public:
        static constexpr GLuint empty_handle = 0;

        Render_buffer() {
            glGenRenderbuffers(1, &handle);
            if (handle == empty_handle) {
                throw std::runtime_error{"glGenRenderbuffers: returned an empty handle"};
            }
        }
        Render_buffer(Render_buffer const&) = delete;
        Render_buffer(Render_buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = empty_handle;
        }
        Render_buffer& operator=(Render_buffer const&) = delete;
        Render_buffer& operator=(Render_buffer&& tmp) {
            GLuint h = tmp.handle;
            tmp.handle = handle;
            handle = h;
            return *this;
        }
        ~Render_buffer() noexcept {
            if (handle != 0) {
                glDeleteRenderbuffers(1, &handle);
            }
        }

        operator GLuint() noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glGenRenderbuffers.xml
    inline Render_buffer GenRenderBuffer() {
        return Render_buffer{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer(Render_buffer& rb) {
        glBindRenderbuffer(GL_RENDERBUFFER, rb);
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
    inline void VertexAttribDivisor(gl::Attribute& attr, GLuint divisor) {
        glVertexAttribDivisor(attr, divisor);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawElements.xhtml
    inline void DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
        glDrawElements(mode, count, type, indices);
    }

    inline void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
        glClearColor(red, green, blue, alpha);
    }

    inline void ClearColor(glm::vec4 const& rgba) {
        glClearColor(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    inline void Viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
        glViewport(x, y, w, h);
    }

    inline void
        FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
        glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    inline void TexParameteri(GLenum target, GLenum pname, GLint param) {
        glTexParameteri(target, pname, param);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    inline void TextureParameteri(GLuint texture, GLenum pname, GLint param) {
        glTextureParameteri(texture, pname, param);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glRenderbufferStorage.xhtml
    inline void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
        glRenderbufferStorage(target, internalformat, width, height);
    }

    inline glm::mat3 normal_matrix(glm::mat4 const& m) {
        return glm::transpose(glm::inverse(m));
    }

    // asserts there are no current OpenGL errors (globally)
    void assert_no_errors(char const* label);

    template<typename T>
    class Array_bufferT final {
        size_t _size = 0;
        gl::Array_buffer _vbo = {};

    public:
        using value_type = T;

        Array_bufferT(T const* begin, T const* end, GLenum usage = GL_STATIC_DRAW) :
            _size{static_cast<size_t>(end - begin)} {

            gl::BindBuffer(_vbo);
            gl::BufferData(_vbo.type, static_cast<long>(_size * sizeof(T)), begin, usage);
        }

        template<typename Container>
        Array_bufferT(Container const& c) : Array_bufferT{c.data(), c.data() + c.size()} {
        }

        operator gl::Array_buffer&() noexcept {
            return _vbo;
        }

        operator gl::Array_buffer const&() const noexcept {
            return _vbo;
        }

        size_t size() const noexcept {
            return _size;
        }

        GLsizei sizei() const noexcept {
            return static_cast<GLsizei>(_size);
        }
    };

    inline void DrawBuffer(GLenum mode) {
        glDrawBuffer(mode);
    }

    template<typename... T>
    inline void DrawBuffers(T... vs) {
        GLenum attachments[sizeof...(vs)] = {static_cast<GLenum>(vs)...};
        glDrawBuffers(sizeof...(vs), attachments);
    }

    inline void assert_current_fbo_complete() {
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }

    inline int GetInteger(GLenum pname) {
        GLint out;
        glGetIntegerv(pname, &out);
        return out;
    }

    inline GLenum GetEnum(GLenum pname) {
        return static_cast<GLenum>(GetInteger(pname));
    }
}

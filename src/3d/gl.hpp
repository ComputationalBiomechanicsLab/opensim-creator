#pragma once

#include <GL/glew.h>
#include <exception>
#include <initializer_list>
#include <string>

#define GL_STRINGIFY(x) #x
#define GL_TOSTRING(x) GL_STRINGIFY(x)
#define GL_SOURCELOC __FILE__ ":" GL_TOSTRING(__LINE__)

namespace gl {

    // an exception that specifically means something has gone wrong in
    // the OpenGL API
    class Opengl_exception final : public std::exception {
        std::string msg;

    public:
        template<typename Str>
        Opengl_exception(Str&& _msg) : msg{_msg} {
        }

        char const* what() const noexcept override;
    };

    constexpr void swap(GLuint& a, GLuint& b) noexcept {
        GLuint v = a;
        a = b;
        b = v;
    }

    constexpr void swap(GLint& a, GLint& b) noexcept {
        GLint v = a;
        a = b;
        b = v;
    }

    // a moveable handle to an OpenGL shader
    class Shader_handle {
        GLuint handle;

    public:
        static constexpr GLuint senteniel = 0;

        explicit Shader_handle(GLenum type) : handle{glCreateShader(type)} {
            if (handle == senteniel) {
                throw Opengl_exception{GL_SOURCELOC ": glCreateShader() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Shader_handle(Shader_handle const&) = delete;

        constexpr Shader_handle(Shader_handle&& tmp) noexcept : handle{tmp.handle} {
            tmp.handle = senteniel;
        }

        Shader_handle& operator=(Shader_handle const&) = delete;

        constexpr Shader_handle& operator=(Shader_handle&& tmp) noexcept {
            swap(handle, tmp.handle);
            return *this;
        }

        ~Shader_handle() noexcept {
            if (handle != senteniel) {
                glDeleteShader(handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // compile a shader from source
    void CompileFromSource(Shader_handle const&, const char* src);

    // a shader of a particular type (e.g. GL_FRAGMENT_SHADER) that owns a
    // shader handle
    template<GLuint ShaderType>
    class Shader {
        Shader_handle handle_;

    public:
        static constexpr GLuint type = ShaderType;

        Shader() : handle_{type} {
        }

        [[nodiscard]] constexpr decltype(handle_.get()) get() const noexcept {
            return handle_.get();
        }

        [[nodiscard]] constexpr Shader_handle& handle() noexcept {
            return handle_;
        }

        [[nodiscard]] constexpr Shader_handle const& handle() const noexcept {
            return handle_;
        }
    };

    class Vertex_shader : public Shader<GL_VERTEX_SHADER> {};
    class Fragment_shader : public Shader<GL_FRAGMENT_SHADER> {};
    class Geometry_shader : public Shader<GL_GEOMETRY_SHADER> {};

    template<typename TShader>
    inline TShader CompileFromSource(const char* src) {
        TShader rv;
        CompileFromSource(rv.handle(), src);
        return rv;
    }

    // an OpenGL program (i.e. n shaders linked into one pipeline)
    class Program final {
        GLuint handle;

    public:
        static constexpr GLuint senteniel = 0;

        Program() : handle{glCreateProgram()} {
            if (handle == senteniel) {
                throw Opengl_exception{GL_SOURCELOC "glCreateProgram() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Program(Program const&) = delete;

        constexpr Program(Program&& tmp) noexcept : handle{tmp.handle} {
            tmp.handle = senteniel;
        }

        Program& operator=(Program const&) = delete;

        constexpr Program& operator=(Program&& tmp) noexcept {
            swap(handle, tmp.handle);
            return *this;
        }

        ~Program() noexcept {
            if (handle != senteniel) {
                glDeleteProgram(handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram(Program const& p) noexcept {
        glUseProgram(p.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram() {
        glUseProgram(static_cast<GLuint>(0));
    }

    inline void AttachShader(Program& p, Shader_handle const& sh) {
        glAttachShader(p.get(), sh.get());
    }

    template<GLuint ShaderType>
    inline void AttachShader(Program& p, Shader<ShaderType> const& s) {
        glAttachShader(p.get(), s.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glLinkProgram.xhtml
    void LinkProgram(Program& prog);

    inline gl::Program CreateProgramFrom(Vertex_shader const& vs, Fragment_shader const& fs) {
        gl::Program p;
        AttachShader(p, vs);
        AttachShader(p, fs);
        LinkProgram(p);
        return p;
    }

    inline gl::Program
        CreateProgramFrom(Vertex_shader const& vs, Fragment_shader const& fs, Geometry_shader const& gs) {

        gl::Program p;
        AttachShader(p, vs);
        AttachShader(p, fs);
        AttachShader(p, gs);
        LinkProgram(p);
        return p;
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    //     *throws on error
    [[nodiscard]] inline GLint GetUniformLocation(Program const& p, GLchar const* name) {
        GLint handle = glGetUniformLocation(p.get(), name);
        if (handle == -1) {
            throw Opengl_exception{std::string{"glGetUniformLocation() failed: cannot get "} + name};
        }
        return handle;
    }

    [[nodiscard]] inline GLint GetAttribLocation(Program const& p, GLchar const* name) {
        GLint handle = glGetAttribLocation(p.get(), name);
        if (handle == -1) {
            throw Opengl_exception{std::string{"glGetAttribLocation() failed: cannot get "} + name};
        }
        return handle;
    }

    // metadata for GLSL data types that are typically bound from the CPU via (e.g.)
    // glVertexAttribPointer
    namespace glsl {
        struct float_ final {
            static constexpr GLint size = 1;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct int_ {
            static constexpr GLint size = 1;
            static constexpr GLenum type = GL_INT;
        };
        struct sampler2d : public int_ {};
        struct sampler2DMS : public int_ {};
        struct samplerCube : public int_ {};
        struct bool_ : public int_ {};
        struct vec2 final {
            static constexpr GLint size = 2;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct vec3 final {
            static constexpr GLint size = 3;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct vec4 final {
            static constexpr GLint size = 4;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct mat4 final {
            static constexpr GLint size = 16;
            static constexpr GLenum type = GL_FLOAT;
            static constexpr size_t els_per_location = 4;
        };
        struct mat3 final {
            static constexpr GLint size = 9;
            static constexpr GLenum type = GL_FLOAT;
            static constexpr size_t els_per_location = 3;
        };
        struct mat4x3 final {
            static constexpr GLint size = 12;
            static constexpr GLenum type = GL_FLOAT;
            static constexpr size_t els_per_location = 3;
        };
    }

    // a uniform shader symbol (e.g. `uniform mat4 uProjectionMatrix`) at a
    // particular location in a linked OpenGL program
    template<typename TGlsl>
    class Uniform_ {
        GLint location;

    public:
        constexpr Uniform_(GLint _location) noexcept : location{_location} {
        }

        Uniform_(Program const& p, GLchar const* name) : location{GetUniformLocation(p, name)} {
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return static_cast<GLuint>(location);
        }

        [[nodiscard]] constexpr GLint geti() const noexcept {
            return static_cast<GLint>(location);
        }
    };

    class Uniform_float : public Uniform_<glsl::float_> {
        using Uniform_::Uniform_;
    };
    class Uniform_int : public Uniform_<glsl::int_> {
        using Uniform_::Uniform_;
    };
    class Uniform_mat4 : public Uniform_<glsl::mat4> {
        using Uniform_::Uniform_;
    };
    class Uniform_mat3 : public Uniform_<glsl::mat3> {
        using Uniform_::Uniform_;
    };
    class Uniform_vec4 : public Uniform_<glsl::vec4> {
        using Uniform_::Uniform_;
    };
    class Uniform_vec3 : public Uniform_<glsl::vec3> {
        using Uniform_::Uniform_;
    };
    class Uniform_vec2 : public Uniform_<glsl::vec2> {
        using Uniform_::Uniform_;
    };
    class Uniform_bool : public Uniform_<glsl::bool_> {
        using Uniform_::Uniform_;
    };
    class Uniform_sampler2d : public Uniform_<glsl::sampler2d> {
        using Uniform_::Uniform_;
    };
    class Uniform_samplerCube : public Uniform_<glsl::samplerCube> {
        using Uniform_::Uniform_;
    };
    class Uniform_sampler2DMS : public Uniform_<glsl::sampler2DMS> {
        using Uniform_::Uniform_;
    };

    // set the value of a `float` uniform in the currently bound program
    inline void Uniform(Uniform_float& u, GLfloat value) noexcept {
        glUniform1f(u.geti(), value);
    }

    // set the value of an `int` uniform in the currently bound program
    inline void Uniform(Uniform_int& u, GLint value) noexcept {
        glUniform1i(u.geti(), value);
    }

    // a uniform that points to a statically-sized array of values in the shader
    //
    // This is just a uniform that points to the first element. The utility of
    // this class is that it disambiguates overloads (so that calling code can
    // assign sequences of values to uniform arrays)
    template<typename TGlsl, int N>
    class Uniform_array final : public Uniform_<TGlsl> {
        static_assert(N >= 0);

    public:
        constexpr Uniform_array(GLint _location) noexcept : Uniform_<TGlsl>{_location} {
        }

        Uniform_array(Program const& p, GLchar const* name) : Uniform_<TGlsl>{p, name} {
        }

        [[nodiscard]] constexpr size_t size() const noexcept {
            return static_cast<size_t>(N);
        }

        [[nodiscard]] constexpr int sizei() const noexcept {
            return N;
        }
    };

    // an attribute shader symbol (e.g. `attribute vec3 aPos`) at at particular
    // location in a linked OpenGL program
    template<typename TGlsl>
    class Attribute {
        GLint location;

    public:
        using glsl_type = TGlsl;

        constexpr Attribute(GLint _location) noexcept : location{_location} {
        }

        Attribute(Program const& p, GLchar const* name) : location{GetAttribLocation(p, name)} {
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return static_cast<GLuint>(location);
        }

        [[nodiscard]] constexpr GLint geti() const noexcept {
            return static_cast<GLint>(location);
        }
    };

    // utility defs for attributes typically used in downstream code
    using Attribute_float = Attribute<glsl::float_>;
    using Attribute_int = Attribute<glsl::int_>;
    using Attribute_vec2 = Attribute<glsl::vec2>;
    using Attribute_vec3 = Attribute<glsl::vec3>;
    using Attribute_vec4 = Attribute<glsl::vec4>;
    using Attribute_mat4 = Attribute<glsl::mat4>;
    using Attribute_mat3 = Attribute<glsl::mat3>;
    using Attribute_mat4x3 = Attribute<glsl::mat4x3>;

    // set the attribute pointer parameters for an attribute, which specifies
    // how the attribute reads its data from an OpenGL buffer
    //
    // this is a higher-level version of `glVertexAttribPointer`, because it
    // also "magically" handles attributes that span multiple locations (e.g. mat4)
    template<typename TGlsl, GLenum SourceType = TGlsl::type>
    inline void
        VertexAttribPointer(Attribute<TGlsl> const& attr, bool normalized, size_t stride, size_t offset) noexcept {
        static_assert(TGlsl::size <= 4 || TGlsl::type == GL_FLOAT);

        GLboolean normgl = normalized ? GL_TRUE : GL_FALSE;
        GLsizei stridegl = static_cast<GLsizei>(stride);
        void* offsetgl = reinterpret_cast<void*>(offset);

        if constexpr (TGlsl::size <= 4) {
            glVertexAttribPointer(attr.get(), TGlsl::size, SourceType, normgl, stridegl, offsetgl);
        } else if constexpr (SourceType == GL_FLOAT) {
            for (unsigned i = 0; i < TGlsl::size / TGlsl::els_per_location; ++i) {
                auto off = reinterpret_cast<void*>(offset + (i * TGlsl::els_per_location * sizeof(float)));
                glVertexAttribPointer(attr.get() + i, TGlsl::els_per_location, SourceType, normgl, stridegl, off);
            }
        }

        // else: not supported: see static_assert above
    }

    // enable an attribute, which effectively makes it load data from the bound
    // OpenGL buffer during a draw call
    //
    // this is a higher-level version of `glEnableVertexAttribArray`, because it
    // also "magically" handles attributes that span multiple locations (e.g. mat4)
    template<typename TGlsl>
    inline void EnableVertexAttribArray(Attribute<TGlsl> const& loc) {
        static_assert(TGlsl::size <= 4 || TGlsl::type == GL_FLOAT);

        if constexpr (TGlsl::size <= 4) {
            glEnableVertexAttribArray(loc.get());
        } else if constexpr (TGlsl::type == GL_FLOAT) {
            for (unsigned i = 0; i < TGlsl::size / TGlsl::els_per_location; ++i) {
                glEnableVertexAttribArray(loc.get() + i);
            }
        }

        // else: not supported: see static_assert above
    }

    // set the attribute divisor, which tells the implementation how to "step"
    // through each attribute during an instanced draw call
    //
    // this is a higher-level version of `glVertexAttribDivisor`, because it
    // also "magically" handles attributes that span multiple locations (e.g. mat4)
    template<typename TGlsl>
    inline void VertexAttribDivisor(Attribute<TGlsl> const& loc, GLuint divisor) {
        static_assert(TGlsl::size <= 4 || TGlsl::type == GL_FLOAT);

        if constexpr (TGlsl::size <= 4) {
            glVertexAttribDivisor(loc.get(), divisor);
        } else if constexpr (TGlsl::type == GL_FLOAT) {
            for (unsigned i = 0; i < TGlsl::size / TGlsl::els_per_location; ++i) {
                glVertexAttribDivisor(loc.get() + i, divisor);
            }
        }
    }

    // a moveable handle to an OpenGL buffer (e.g. GL_ARRAY_BUFFER)
    class Buffer_handle {
        GLuint handle;

    public:
        static constexpr GLuint senteniel = static_cast<GLuint>(-1);

        Buffer_handle() {
            glGenBuffers(1, &handle);
            if (handle == senteniel) {
                throw Opengl_exception{GL_SOURCELOC "glGenBuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Buffer_handle(Buffer_handle const&) = delete;

        constexpr Buffer_handle(Buffer_handle&& tmp) noexcept : handle{tmp.handle} {
            tmp.handle = senteniel;
        }

        Buffer_handle& operator=(Buffer_handle const&) = delete;

        constexpr Buffer_handle& operator=(Buffer_handle&& tmp) noexcept {
            swap(handle, tmp.handle);
            return *this;
        }

        ~Buffer_handle() noexcept {
            if (handle != senteniel) {
                glDeleteBuffers(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // a buffer handle that is locked against a particular type (e.g. GL_ELEMENT_ARRAY_BUFFER)
    template<GLenum BufferType>
    class Typed_buffer_handle : public Buffer_handle {
        using Buffer_handle::Buffer_handle;

    public:
        static constexpr GLenum buffer_type = BufferType;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    inline void BindBuffer(GLenum target, Buffer_handle const& handle) noexcept {
        glBindBuffer(target, handle.get());
    }

    template<GLenum BufferType>
    inline void BindBuffer(Typed_buffer_handle<BufferType> const& handle) {
        glBindBuffer(BufferType, handle.get());
    }

    template<GLenum BufferType>
    inline void UnbindBuffer(Typed_buffer_handle<BufferType> const&) {
        glBindBuffer(BufferType, 0);
    }

    inline void BufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
        glBufferData(target, size, data, usage);
    }

    // an OpenGL buffer with compile-time known:
    //
    // - user type (T)
    // - OpenGL type (BufferType, e.g. GL_ARRAY_BUFFER)
    // - usage (e.g. GL_STATIC_DRAW)
    //
    // must be a trivially copyable type with a standard layout, because its
    // data transfers onto the GPU
    template<typename T, GLenum BufferType, GLenum Usage>
    class Buffer : public Typed_buffer_handle<BufferType> {
        size_t size_;

    public:
        static_assert(std::is_trivially_copyable<T>::value);
        static_assert(std::is_standard_layout<T>::value);

        using value_type = T;
        static constexpr GLenum buffer_type = BufferType;

        Buffer() = default;

        Buffer(T const* begin, size_t n) : Typed_buffer_handle<BufferType>{}, size_{n} {
            BindBuffer(*this);
            BufferData(buffer_type, sizeof(T) * n, begin, Usage);
        }

        template<typename Collection>
        Buffer(Collection const& c) : Buffer{c.data(), c.size()} {
        }

        Buffer(std::initializer_list<T> lst) : Buffer{lst.begin(), lst.size()} {
        }

        template<size_t N>
        Buffer(T const (&arr)[N]) : Buffer{arr, N} {
        }

        [[nodiscard]] constexpr size_t size() const noexcept {
            return size;
        }

        [[nodiscard]] constexpr GLsizei sizei() const noexcept {
            return static_cast<GLsizei>(size_);
        }

        void assign(T const* begin, size_t n) noexcept {
            BindBuffer(*this);
            BufferData(buffer_type, sizeof(T) * n, begin, Usage);
            size_ = n;
        }

        template<typename Container>
        void assign(Container const& c) noexcept {
            assign(c.data(), c.size());
        }

        template<size_t N>
        void assign(T const (&arr)[N]) {
            assign(arr, N);
        }
    };

    template<typename T, GLenum Usage = GL_STATIC_DRAW>
    class Array_buffer : public Buffer<T, GL_ARRAY_BUFFER, Usage> {
        using Buffer<T, GL_ARRAY_BUFFER, Usage>::Buffer;
    };

    template<typename T, GLenum Usage = GL_STATIC_DRAW>
    class Element_array_buffer : public Buffer<T, GL_ELEMENT_ARRAY_BUFFER, Usage> {
        static_assert(std::is_unsigned_v<T>, "element indicies should be unsigned integers");
        static_assert(sizeof(T) <= 4);

        using Buffer<T, GL_ELEMENT_ARRAY_BUFFER, Usage>::Buffer;
    };

    template<typename T, GLenum Usage = GL_STATIC_DRAW>
    class Pixel_pack_buffer : public Buffer<T, GL_PIXEL_PACK_BUFFER, Usage> {
        using Buffer<T, GL_PIXEL_PACK_BUFFER, Usage>::Buffer;
    };

    template<typename Buffer>
    inline void BindBuffer(Buffer const& buf) noexcept {
        glBindBuffer(Buffer::buffer_type, buf.get());
    }

    // returns an OpenGL enum that describes the provided (integral) type
    // argument, so that the index type to an element-based drawcall can
    // be computed at compile-time
    template<typename T>
    inline constexpr GLenum index_type() {
        static_assert(std::is_integral_v<T>, "element indices are integers");
        static_assert(std::is_unsigned_v<T>, "element indices are unsigned data types (in the GL spec)");
        static_assert(sizeof(T) <= 4);

        switch (sizeof(T)) {
        case 1:
            return GL_UNSIGNED_BYTE;
        case 2:
            return GL_UNSIGNED_SHORT;
        case 4:
            return GL_UNSIGNED_INT;
        default:
            return GL_UNSIGNED_INT;
        }
    }

    // utility overload of index_type specifically for EBOs (the most common
    // use-case in downstream code)
    template<typename T>
    inline constexpr GLenum index_type(gl::Element_array_buffer<T> const&) {
        return index_type<T>();
    }

    // a handle to an OpenGL VAO with RAII semantics for glGenVertexArrays etc.
    class Vertex_array final {
        GLuint handle;

    public:
        static constexpr GLuint senteniel = -1;

        Vertex_array() {
            glGenVertexArrays(1, &handle);
            if (handle == senteniel) {
                throw Opengl_exception{GL_SOURCELOC "glGenVertexArrays() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Vertex_array(Vertex_array const&) = delete;

        constexpr Vertex_array(Vertex_array&& tmp) noexcept : handle{tmp.handle} {
            tmp.handle = senteniel;
        }

        Vertex_array& operator=(Vertex_array const&) = delete;

        constexpr Vertex_array& operator=(Vertex_array&& tmp) noexcept {
            swap(handle, tmp.handle);
            return *this;
        }

        ~Vertex_array() noexcept {
            if (handle == static_cast<GLuint>(-1)) {
                glDeleteVertexArrays(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray(Vertex_array& vao) {
        glBindVertexArray(vao.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray() {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    // moveable RAII handle to an OpenGL texture (e.g. GL_TEXTURE_2D)
    class Texture_handle {
        GLuint handle;

    public:
        static constexpr GLuint senteniel = static_cast<GLuint>(-1);

        Texture_handle() {
            glGenTextures(1, &handle);
            if (handle == senteniel) {
                throw Opengl_exception{GL_SOURCELOC "glGenTextures() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Texture_handle(Texture_handle const&) = delete;

        constexpr Texture_handle(Texture_handle&& tmp) noexcept : handle{tmp.handle} {
            tmp.handle = senteniel;
        }

        Texture_handle& operator=(Texture_handle const&) = delete;

        constexpr Texture_handle& operator=(Texture_handle&& tmp) noexcept {
            swap(handle, tmp.handle);
            return *this;
        }

        ~Texture_handle() noexcept {
            if (handle != senteniel) {
                glDeleteTextures(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glActiveTexture.xhtml
    inline void ActiveTexture(GLenum texture) {
        glActiveTexture(texture);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture(GLenum target, Texture_handle const& texture) {
        glBindTexture(target, texture.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // moveable RAII handle to an OpenGL texture with compile-time known type
    template<GLenum TextureType>
    class Texture {
        Texture_handle handle_;

    public:
        static constexpr GLenum type = TextureType;

        [[nodiscard]] constexpr decltype(handle_.get()) get() const noexcept {
            return handle_.get();
        }

        constexpr Texture_handle const& handle() const noexcept {
            return handle_;
        }

        constexpr Texture_handle& handle() noexcept {
            return handle_;
        }
    };

    class Texture_2d : public Texture<GL_TEXTURE_2D> {};
    class Texture_cubemap : public Texture<GL_TEXTURE_CUBE_MAP> {};
    class Texture_2d_multisample : public Texture<GL_TEXTURE_2D_MULTISAMPLE> {};

    template<typename Texture>
    inline void BindTexture(Texture const& t) noexcept {
        glBindTexture(t.type, t.get());
    }

    // moveable RAII handle to an OpenGL framebuffer (i.e. a render target)
    class Frame_buffer final {
        GLuint handle;

    public:
        static constexpr GLuint senteniel = static_cast<GLuint>(-1);

        Frame_buffer() {
            glGenFramebuffers(1, &handle);
            if (handle == senteniel) {
                throw Opengl_exception{GL_SOURCELOC "glGenFramebuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Frame_buffer(Frame_buffer const&) = delete;

        constexpr Frame_buffer(Frame_buffer&& tmp) noexcept : handle{tmp.handle} {
            tmp.handle = senteniel;
        }

        Frame_buffer& operator=(Frame_buffer const&) = delete;

        constexpr Frame_buffer& operator=(Frame_buffer&& tmp) noexcept {
            swap(handle, tmp.handle);
            return *this;
        }

        ~Frame_buffer() noexcept {
            if (handle != senteniel) {
                glDeleteFramebuffers(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFramebuffer(GLenum target, Frame_buffer const& fb) {
        glBindFramebuffer(target, fb.get());
    }

    // bind to the main Window FBO for the current OpenGL context
    struct Window_fbo final {};
    static constexpr Window_fbo window_fbo{};
    inline void BindFramebuffer(GLenum target, Window_fbo) noexcept {
        glBindFramebuffer(target, 0);
    }

    // assign a 2D texture to the framebuffer (so that subsequent draws/reads
    // to/from the FBO use the texture)
    template<typename Texture>
    inline void FramebufferTexture2D(GLenum target, GLenum attachment, Texture const& t, GLint level) noexcept {
        glFramebufferTexture2D(target, attachment, t.type, t.get(), level);
    }

    // moveable RAII handle to an OpenGL render buffer
    class Render_buffer final {
        GLuint handle;

    public:
        static constexpr GLuint senteniel = 0;

        Render_buffer() {
            glGenRenderbuffers(1, &handle);
            if (handle == senteniel) {
                throw Opengl_exception{GL_SOURCELOC "glGenRenderBuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Render_buffer(Render_buffer const&) = delete;

        constexpr Render_buffer(Render_buffer&& tmp) noexcept : handle{tmp.handle} {
            tmp.handle = senteniel;
        }

        Render_buffer& operator=(Render_buffer const&) = delete;

        constexpr Render_buffer& operator=(Render_buffer&& tmp) noexcept {
            swap(handle, tmp.handle);
            return *this;
        }

        ~Render_buffer() noexcept {
            if (handle != senteniel) {
                glDeleteRenderbuffers(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer(Render_buffer& rb) {
        glBindRenderbuffer(GL_RENDERBUFFER, rb.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer() {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    inline void FramebufferRenderbuffer(GLenum target, GLenum attachment, Render_buffer const& rb) {
        glFramebufferRenderbuffer(target, attachment, GL_RENDERBUFFER, rb.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glRenderbufferStorage.xhtml
    inline void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
        glRenderbufferStorage(target, internalformat, width, height);
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
    template<typename Attribute>
    inline void VertexAttribDivisor(Attribute loc, GLuint divisor) noexcept {
        glVertexAttribDivisor(loc.get(), divisor);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawElements.xhtml
    inline void DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
        glDrawElements(mode, count, type, indices);
    }

    inline void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
        glClearColor(red, green, blue, alpha);
    }

    inline void Viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
        glViewport(x, y, w, h);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    inline void TexParameteri(GLenum target, GLenum pname, GLint param) {
        glTexParameteri(target, pname, param);
    }

    inline void TexImage2D(
        GLenum target,
        GLint level,
        GLint internalformat,
        GLsizei width,
        GLsizei height,
        GLint border,
        GLenum format,
        GLenum type,
        const void* pixels) {
        glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    template<typename Texture>
    inline void TextureParameteri(Texture const& texture, GLenum pname, GLint param) {
        glTextureParameteri(texture.raw_handle(), pname, param);
    }

    template<GLenum E>
    inline constexpr unsigned texture_index() {
        static_assert(GL_TEXTURE0 <= E && E <= GL_TEXTURE30);
        return E - GL_TEXTURE0;
    }

    template<typename... T>
    inline void DrawBuffers(T... vs) {
        GLenum attachments[sizeof...(vs)] = {static_cast<GLenum>(vs)...};
        glDrawBuffers(sizeof...(vs), attachments);
    }

    inline bool is_current_fbo_complete() {
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
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

    inline void DrawBuffer(GLenum mode) {
        glDrawBuffer(mode);
    }

    struct Error_span final {
        size_t n;
        char const** first;
    };

    Error_span pop_opengl_errors();

    inline int GetInteger(GLenum pname) {
        GLint out;
        glGetIntegerv(pname, &out);
        return out;
    }

    inline GLenum GetEnum(GLenum pname) {
        return static_cast<GLenum>(GetInteger(pname));
    }

    inline void Enable(GLenum cap) {
        glEnable(cap);
    }

    inline void Disable(GLenum cap) {
        glDisable(cap);
    }
}

#pragma once

#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/Utils/Concepts.h>

#include <GL/glew.h>

#include <concepts>
#include <cstddef>
#include <exception>
#include <limits>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

// gl: convenience C++ bindings to OpenGL
namespace osc::gl
{
    // an exception that specifically means something has gone wrong in
    // the OpenGL API
    class OpenGlException final : public std::exception {
    public:
        OpenGlException(std::string s) : m_Msg{std::move(s)}
        {
        }

        char const* what() const noexcept final
        {
            return m_Msg.c_str();
        }

    private:
        std::string m_Msg;
    };

    // a moveable handle to an OpenGL shader
    class ShaderHandle {
    public:
        explicit ShaderHandle(GLenum type) :
            m_ShaderHandle{glCreateShader(type)}
        {
            if (m_ShaderHandle == c_EmptyShaderSenteniel)
            {
                throw OpenGlException{"glCreateShader() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        ShaderHandle(ShaderHandle const&) = delete;

        ShaderHandle(ShaderHandle&& tmp) noexcept :
            m_ShaderHandle{std::exchange(tmp.m_ShaderHandle, c_EmptyShaderSenteniel)}
        {
        }

        ShaderHandle& operator=(ShaderHandle const&) = delete;

        ShaderHandle& operator=(ShaderHandle&& tmp) noexcept
        {
            std::swap(m_ShaderHandle, tmp.m_ShaderHandle);
            return *this;
        }

        ~ShaderHandle() noexcept
        {
            if (m_ShaderHandle != c_EmptyShaderSenteniel)
            {
                glDeleteShader(m_ShaderHandle);
            }
        }

        GLuint get() const
        {
            return m_ShaderHandle;
        }

    private:
        static constexpr GLuint c_EmptyShaderSenteniel = 0;
        GLuint m_ShaderHandle;
    };

    // compile a shader from source
    void CompileFromSource(ShaderHandle const&, GLchar const* src);

    // a shader of a particular type (e.g. GL_FRAGMENT_SHADER) that owns a
    // shader handle
    template<GLuint ShaderType>
    class Shader {
    public:
        static constexpr GLuint type = ShaderType;

        Shader() : m_ShaderHandle{type} {}

        GLuint get() const
        {
            return m_ShaderHandle.get();
        }

        ShaderHandle& handle()
        {
            return m_ShaderHandle;
        }

        ShaderHandle const& handle() const
        {
            return m_ShaderHandle;
        }

    private:
        ShaderHandle m_ShaderHandle;
    };

    class VertexShader : public Shader<GL_VERTEX_SHADER> {};
    class FragmentShader : public Shader<GL_FRAGMENT_SHADER> {};
    class GeometryShader : public Shader<GL_GEOMETRY_SHADER> {};

    template<typename TShader>
    inline TShader CompileFromSource(GLchar const* src)
    {
        TShader rv;
        CompileFromSource(rv.handle(), src);
        return rv;
    }

    // an OpenGL program (i.e. n shaders linked into one pipeline)
    class Program final {
    public:
        Program() : m_ProgramHandle{glCreateProgram()}
        {
            if (m_ProgramHandle == c_EmptyProgramSenteniel)
            {
                throw OpenGlException{"glCreateProgram() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Program(Program const&) = delete;

        Program(Program&& tmp) noexcept :
            m_ProgramHandle{std::exchange(tmp.m_ProgramHandle, c_EmptyProgramSenteniel)}
        {
        }

        Program& operator=(Program const&) = delete;

        Program& operator=(Program&& tmp) noexcept
        {
            std::swap(m_ProgramHandle, tmp.m_ProgramHandle);
            return *this;
        }

        ~Program() noexcept
        {
            if (m_ProgramHandle != c_EmptyProgramSenteniel)
            {
                glDeleteProgram(m_ProgramHandle);
            }
        }

        GLuint get() const
        {
            return m_ProgramHandle;
        }

    private:
        static constexpr GLuint c_EmptyProgramSenteniel = 0;
        GLuint m_ProgramHandle;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram(Program const& p)
    {
        glUseProgram(p.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram()
    {
        glUseProgram(static_cast<GLuint>(0));
    }

    inline void AttachShader(Program& p, ShaderHandle const& sh)
    {
        glAttachShader(p.get(), sh.get());
    }

    template<GLuint ShaderType>
    inline void AttachShader(Program& p, Shader<ShaderType> const& s)
    {
        glAttachShader(p.get(), s.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glLinkProgram.xhtml
    void LinkProgram(Program& prog);

    inline gl::Program CreateProgramFrom(VertexShader const& vs, FragmentShader const& fs)
    {
        gl::Program p;
        AttachShader(p, vs);
        AttachShader(p, fs);
        LinkProgram(p);
        return p;
    }

    inline gl::Program CreateProgramFrom(VertexShader const& vs, FragmentShader const& fs, GeometryShader const& gs)
    {
        gl::Program p;
        AttachShader(p, vs);
        AttachShader(p, fs);
        AttachShader(p, gs);
        LinkProgram(p);
        return p;
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    //     - throws on error
    [[nodiscard]] inline GLint GetUniformLocation(Program const& p, GLchar const* name)
    {
        GLint const handle = glGetUniformLocation(p.get(), name);
        if (handle == -1)
        {
            throw OpenGlException{std::string{"glGetUniformLocation() failed: cannot get "} + name};
        }
        return handle;
    }

    [[nodiscard]] inline GLint GetAttribLocation(Program const& p, GLchar const* name)
    {
        GLint const handle = glGetAttribLocation(p.get(), name);
        if (handle == -1)
        {
            throw OpenGlException{std::string{"glGetAttribLocation() failed: cannot get "} + name};
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
            static constexpr size_t elementsPerLocation = 4;
        };
        struct mat3 final {
            static constexpr GLint size = 9;
            static constexpr GLenum type = GL_FLOAT;
            static constexpr size_t elementsPerLocation = 3;
        };
        struct mat4x3 final {
            static constexpr GLint size = 12;
            static constexpr GLenum type = GL_FLOAT;
            static constexpr size_t elementsPerLocation = 3;
        };
    }

    // a uniform shader symbol (e.g. `uniform mat4 uProjectionMatrix`) at a
    // particular location in a linked OpenGL program
    template<typename TGlsl>
    class Uniform_ {
    public:
        constexpr Uniform_(GLint _location) :
            m_UniformLocation{_location}
        {
        }

        Uniform_(Program const& p, GLchar const* name) :
            m_UniformLocation{GetUniformLocation(p, name)}
        {
        }

        [[nodiscard]] constexpr GLuint get() const
        {
            return static_cast<GLuint>(m_UniformLocation);
        }

        [[nodiscard]] constexpr GLint geti() const
        {
            return m_UniformLocation;
        }

    private:
        GLint m_UniformLocation;
    };

    class UniformFloat : public Uniform_<glsl::float_> {
        using Uniform_::Uniform_;
    };
    class UniformInt : public Uniform_<glsl::int_> {
        using Uniform_::Uniform_;
    };
    class UniformMat4 : public Uniform_<glsl::mat4> {
        using Uniform_::Uniform_;
    };
    class UniformMat3 : public Uniform_<glsl::mat3> {
        using Uniform_::Uniform_;
    };
    class UniformVec4 : public Uniform_<glsl::vec4> {
        using Uniform_::Uniform_;
    };
    class UniformVec3 : public Uniform_<glsl::vec3> {
        using Uniform_::Uniform_;
    };
    class UniformVec2 : public Uniform_<glsl::vec2> {
        using Uniform_::Uniform_;
    };
    class UniformBool : public Uniform_<glsl::bool_> {
        using Uniform_::Uniform_;
    };
    class UniformSampler2D : public Uniform_<glsl::sampler2d> {
        using Uniform_::Uniform_;
    };
    class UniformSamplerCube : public Uniform_<glsl::samplerCube> {
        using Uniform_::Uniform_;
    };
    class UniformSampler2DMS : public Uniform_<glsl::sampler2DMS> {
        using Uniform_::Uniform_;
    };

    // set the value of a `float` uniform in the currently bound program
    inline void Uniform(UniformFloat& u, GLfloat value)
    {
        glUniform1f(u.geti(), value);
    }

    // set the value of an `GLint` uniform in the currently bound program
    inline void Uniform(UniformInt& u, GLint value)
    {
        glUniform1i(u.geti(), value);
    }

    // set the value of an array-like uniform `GLint`
    inline void Uniform(UniformInt const& u, GLsizei n, GLint const* data)
    {
        glUniform1iv(u.geti(), n, data);
    }

    // set the value of a `vec3` uniform
    inline void Uniform(UniformVec3& u, float x, float y, float z)
    {
        glUniform3f(u.geti(), x, y, z);
    }

    // set the value of a `vec3` uniform
    inline void Uniform(UniformVec3& u, float const vs[3])
    {
        glUniform3fv(u.geti(), 1, vs);
    }

    // set the value of a `sampler2D` uniform
    inline void Uniform(UniformSampler2D& u, GLint v)
    {
        glUniform1i(u.geti(), v);
    }

    inline void Uniform(UniformSamplerCube& u, GLint v)
    {
        glUniform1i(u.geti(), v);
    }

    // set the value of an `sampler2DMS` uniform
    inline void Uniform(UniformSampler2DMS& u, GLint v)
    {
        glUniform1i(u.geti(), v);
    }

    // set the value of a `bool` uniform
    inline void Uniform(UniformBool& u, bool v)
    {
        glUniform1i(u.geti(), v);
    }

    // a uniform that points to a statically-sized array of values in the shader
    //
    // This is just a uniform that points to the first element. The utility of
    // this class is that it disambiguates overloads (so that calling code can
    // assign sequences of values to uniform arrays)
    template<typename TGlsl, size_t N>
    class UniformArray final : public Uniform_<TGlsl> {
        static_assert(N >= 0);

    public:
        constexpr UniformArray(GLint location) :
            Uniform_<TGlsl>{location}
        {
        }

        UniformArray(Program const& p, GLchar const* name) :
            Uniform_<TGlsl>{p, name}
        {
        }

        [[nodiscard]] constexpr size_t size() const
        {
            return N;
        }
    };

    inline void Uniform(UniformMat3& u, Mat3 const& mat)
    {
        glUniformMatrix3fv(u.geti(), 1, false, value_ptr(mat));
    }

    inline void Uniform(UniformVec4& u, Vec4 const& v)
    {
        glUniform4fv(u.geti(), 1, value_ptr(v));
    }

    inline void Uniform(UniformVec3& u, Vec3 const& v)
    {
        glUniform3fv(u.geti(), 1, value_ptr(v));
    }

    inline void Uniform(UniformVec3& u, std::span<Vec3 const> vs)
    {
        static_assert(sizeof(Vec3) == 3 * sizeof(GLfloat));
        glUniform3fv(u.geti(), static_cast<GLsizei>(vs.size()), value_ptr(vs.front()));
    }

    // set a uniform array of vec3s from a userspace container type (e.g. vector<Vec3>)
    template<std::ranges::contiguous_range Range, size_t N>
    inline void Uniform(UniformArray<glsl::vec3, N>& u, Range& range)
        requires std::same_as<typename Range::value_type, Vec3>
    {
        OSC_ASSERT(std::ranges::size(range) == N);
        glUniform3fv(u.geti(), static_cast<GLsizei>(std::ranges::size(range)), ValuePtr(*std::ranges::data(range)));
    }

    inline void Uniform(UniformMat4& u, Mat4 const& mat)
    {
        glUniformMatrix4fv(u.geti(), 1, false, value_ptr(mat));
    }

    inline void Uniform(UniformMat4& u, std::span<Mat4 const> ms)
    {
        static_assert(sizeof(Mat4) == 16 * sizeof(GLfloat));
        glUniformMatrix4fv(u.geti(), static_cast<GLsizei>(ms.size()), false, value_ptr(ms.front()));
    }

    inline void Uniform(UniformVec2& u, Vec2 const& v)
    {
        glUniform2fv(u.geti(), 1, value_ptr(v));
    }

    inline void Uniform(UniformVec2& u, std::span<Vec2 const> vs)
    {
        static_assert(sizeof(Vec2) == 2 * sizeof(GLfloat));

        glUniform2fv(
            u.geti(),
            static_cast<GLsizei>(vs.size()),
            value_ptr(vs.front())
        );
    }

    template<std::ranges::contiguous_range Range, size_t N>
    void Uniform(UniformArray<glsl::vec2, N>& u, Range const& range)
        requires std::same_as<typename Range::value_type, Vec2>
    {
        glUniform2fv(
            u.geti(),
            static_cast<GLsizei>(std::ranges::size(range)),
            ValuePtr(std::ranges::data(range))
        );
    }

    // an attribute shader symbol (e.g. `attribute vec3 aPos`) at at particular
    // location in a linked OpenGL program
    template<typename TGlsl>
    class Attribute {
    public:
        using glsl_type = TGlsl;

        constexpr Attribute(GLint location) :
            m_AttributeLocation{location}
        {
        }

        Attribute(Program const& p, GLchar const* name) :
            m_AttributeLocation{GetAttribLocation(p, name)}
        {
        }

        [[nodiscard]] constexpr GLuint get() const
        {
            return static_cast<GLuint>(m_AttributeLocation);
        }

        [[nodiscard]] constexpr GLint geti() const
        {
            return m_AttributeLocation;
        }

    private:
        GLint m_AttributeLocation;
    };

    // utility defs for attributes typically used in downstream code
    using AttributeFloat = Attribute<glsl::float_>;
    using AttributeInt = Attribute<glsl::int_>;
    using AttributeVec2 = Attribute<glsl::vec2>;
    using AttributeVec3 = Attribute<glsl::vec3>;
    using AttributeVec4 = Attribute<glsl::vec4>;
    using AttributeMat4 = Attribute<glsl::mat4>;
    using AttributeMat3 = Attribute<glsl::mat3>;
    using AttributeMat4x3 = Attribute<glsl::mat4x3>;

    // set the attribute pointer parameters for an attribute, which specifies
    // how the attribute reads its data from an OpenGL buffer
    //
    // this is a higher-level version of `glVertexAttribPointer`, because it
    // also "magically" handles attributes that span multiple locations (e.g. mat4)
    template<typename TGlsl, GLenum SourceType = TGlsl::type>
    inline void VertexAttribPointer(Attribute<TGlsl> const& attr,
                                    bool normalized,
                                    size_t stride,
                                    size_t offset)
    {
        static_assert(TGlsl::size <= 4 || TGlsl::type == GL_FLOAT);

        GLboolean const normgl = normalized ? GL_TRUE : GL_FALSE;
        GLsizei const stridegl = static_cast<GLsizei>(stride);

        if constexpr (TGlsl::size <= 4)
        {
            glVertexAttribPointer(
                attr.get(),
                TGlsl::size,
                SourceType,
                normgl,
                stridegl,
                cpp20::bit_cast<void*>(offset)
            );
        }
        else if constexpr (SourceType == GL_FLOAT)
        {
            for (size_t i = 0; i < TGlsl::size / TGlsl::elementsPerLocation; ++i)
            {
                glVertexAttribPointer(
                    attr.get() + static_cast<GLuint>(i),
                    TGlsl::elementsPerLocation,
                    SourceType,
                    normgl,
                    stridegl,
                    cpp20::bit_cast<void*>(offset + (i * TGlsl::elementsPerLocation * sizeof(float)))
                );
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
    inline void EnableVertexAttribArray(Attribute<TGlsl> const& loc)
    {
        static_assert(TGlsl::size <= 4 || TGlsl::type == GL_FLOAT);

        if constexpr (TGlsl::size <= 4)
        {
            glEnableVertexAttribArray(loc.get());
        }
        else if constexpr (TGlsl::type == GL_FLOAT)
        {
            for (size_t i = 0; i < TGlsl::size / TGlsl::elementsPerLocation; ++i)
            {
                glEnableVertexAttribArray(loc.get() + static_cast<GLuint>(i));
            }
        }

        // else: not supported: see static_assert above
    }

    template<typename TGlsl>
    inline void DisableVertexAttribArray(Attribute<TGlsl> const& loc)
    {
        static_assert(TGlsl::size <= 4 || TGlsl::type == GL_FLOAT);

        if constexpr (TGlsl::size <= 4)
        {
            glDisableVertexAttribArray(loc.get());
        }
        else if constexpr (TGlsl::type == GL_FLOAT)
        {
            for (size_t i = 0; i < TGlsl::size / TGlsl::elementsPerLocation; ++i)
            {
                glDisableVertexAttribArray(loc.get() + static_cast<GLuint>(i));
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
    inline void VertexAttribDivisor(Attribute<TGlsl> const& loc, GLuint divisor)
    {
        static_assert(TGlsl::size <= 4 || TGlsl::type == GL_FLOAT);

        if constexpr (TGlsl::size <= 4)
        {
            glVertexAttribDivisor(loc.get(), divisor);
        }
        else if constexpr (TGlsl::type == GL_FLOAT)
        {
            for (size_t i = 0; i < TGlsl::size / TGlsl::elementsPerLocation; ++i)
            {
                glVertexAttribDivisor(loc.get() + static_cast<GLuint>(i), divisor);
            }
        }
    }

    // a moveable handle to an OpenGL buffer (e.g. GL_ARRAY_BUFFER)
    class BufferHandle {
    public:
        BufferHandle()
        {
            glGenBuffers(1, &m_BufferHandle);

            if (m_BufferHandle == c_EmptyBufferHandleSenteniel)
            {
                throw OpenGlException{"glGenBuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        BufferHandle(BufferHandle const&) = delete;

        BufferHandle(BufferHandle&& tmp) noexcept :
            m_BufferHandle{std::exchange(tmp.m_BufferHandle, c_EmptyBufferHandleSenteniel)}
        {
        }

        BufferHandle& operator=(BufferHandle const&) = delete;

        BufferHandle& operator=(BufferHandle&& tmp) noexcept
        {
            std::swap(m_BufferHandle, tmp.m_BufferHandle);
            return *this;
        }

        ~BufferHandle() noexcept
        {
            if (m_BufferHandle != c_EmptyBufferHandleSenteniel)
            {
                glDeleteBuffers(1, &m_BufferHandle);
            }
        }

        GLuint get() const
        {
            return m_BufferHandle;
        }

    private:
        static constexpr GLuint c_EmptyBufferHandleSenteniel = static_cast<GLuint>(-1);
        GLuint m_BufferHandle;
    };

    // a buffer handle that is locked against a particular type (e.g. GL_ELEMENT_ARRAY_BUFFER)
    template<GLenum TBuffer>
    class TypedBufferHandle : public BufferHandle {
        using BufferHandle::BufferHandle;

    public:
        static constexpr GLenum BufferType = TBuffer;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    inline void BindBuffer(GLenum target, BufferHandle const& handle)
    {
        glBindBuffer(target, handle.get());
    }

    template<GLenum TBuffer>
    inline void BindBuffer(TypedBufferHandle<TBuffer> const& handle)
    {
        glBindBuffer(TBuffer, handle.get());
    }

    template<GLenum TBuffer>
    inline void UnbindBuffer(TypedBufferHandle<TBuffer> const&)
    {
        glBindBuffer(TBuffer, 0);
    }

    inline void BufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
    {
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
    template<BitCastable T, GLenum TBuffer, GLenum Usage>
    class Buffer : public TypedBufferHandle<TBuffer> {
    public:
        using value_type = T;
        static constexpr GLenum BufferType = TBuffer;

        template<std::ranges::contiguous_range Range>
        void assign(Range const& range)
        {
            BindBuffer(*this);
            BufferData(BufferType, sizeof(T) * std::ranges::size(range), std::ranges::data(range), Usage);
        }
    };

    template<BitCastable T, GLenum Usage = GL_STATIC_DRAW>
    class ArrayBuffer : public Buffer<T, GL_ARRAY_BUFFER, Usage> {
        using Buffer<T, GL_ARRAY_BUFFER, Usage>::Buffer;
    };

    template<typename T>
    concept ElementIndex = IsAnyOf<uint16_t, uint32_t>;

    template<ElementIndex T, GLenum Usage = GL_STATIC_DRAW>
    class ElementArrayBuffer : public Buffer<T, GL_ELEMENT_ARRAY_BUFFER, Usage> {
        using Buffer<T, GL_ELEMENT_ARRAY_BUFFER, Usage>::Buffer;
    };

    template<BitCastable T, GLenum Usage = GL_STATIC_DRAW>
    class PixelPackBuffer : public Buffer<T, GL_PIXEL_PACK_BUFFER, Usage> {
        using Buffer<T, GL_PIXEL_PACK_BUFFER, Usage>::Buffer;
    };

    template<typename Buffer>
    inline void BindBuffer(Buffer const& buf)
    {
        glBindBuffer(Buffer::BufferType, buf.get());
    }

    // returns an OpenGL enum that describes the provided (integral) type
    // argument, so that the index type to an element-based drawcall can
    // be computed at compile-time
    template<std::unsigned_integral T>
    inline constexpr GLenum indexType();

    template<>
    inline constexpr GLenum indexType<uint8_t>() { return GL_UNSIGNED_BYTE; }

    template<>
    inline constexpr GLenum indexType<uint16_t>() { return GL_UNSIGNED_SHORT; }

    template<>
    inline constexpr GLenum indexType<uint32_t>() { return GL_UNSIGNED_INT; }

    // utility overload of index_type specifically for EBOs (the most common
    // use-case in downstream code)
    template<typename T>
    inline constexpr GLenum indexType(gl::ElementArrayBuffer<T> const&)
    {
        return indexType<T>();
    }

    // a handle to an OpenGL VAO with RAII semantics for glGenVertexArrays etc.
    class VertexArray final {
    public:
        VertexArray()
        {
            glGenVertexArrays(1, &m_VaoHandle);

            if (m_VaoHandle == c_EmptyVAOHandleSenteniel)
            {
                throw OpenGlException{"glGenVertexArrays() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        VertexArray(VertexArray const&) = delete;

        VertexArray(VertexArray&& tmp) noexcept :
            m_VaoHandle{std::exchange(tmp.m_VaoHandle, c_EmptyVAOHandleSenteniel)}
        {
        }

        VertexArray& operator=(VertexArray const&) = delete;

        VertexArray& operator=(VertexArray&& tmp) noexcept
        {
            std::swap(m_VaoHandle, tmp.m_VaoHandle);
            return *this;
        }

        ~VertexArray() noexcept
        {
            if (m_VaoHandle != c_EmptyVAOHandleSenteniel)
            {
                glDeleteVertexArrays(1, &m_VaoHandle);
            }
        }

        GLuint get() const
        {
            return m_VaoHandle;
        }

    private:
        static constexpr GLuint c_EmptyVAOHandleSenteniel = std::numeric_limits<GLuint>::max();
        GLuint m_VaoHandle;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray(VertexArray const& vao)
    {
        glBindVertexArray(vao.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray()
    {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    // moveable RAII handle to an OpenGL texture (e.g. GL_TEXTURE_2D)
    class TextureHandle {
    public:
        TextureHandle()
        {
            glGenTextures(1, &m_TextureHandle);

            if (m_TextureHandle == c_EmptyTextureHandleSenteniel)
            {
                throw OpenGlException{"glGenTextures() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        TextureHandle(TextureHandle const&) = delete;

        TextureHandle(TextureHandle&& tmp) noexcept :
            m_TextureHandle{std::exchange(tmp.m_TextureHandle, c_EmptyTextureHandleSenteniel)}
        {
        }

        TextureHandle& operator=(TextureHandle const&) = delete;

        TextureHandle& operator=(TextureHandle&& tmp) noexcept
        {
            std::swap(m_TextureHandle, tmp.m_TextureHandle);
            return *this;
        }

        ~TextureHandle() noexcept
        {
            if (m_TextureHandle != c_EmptyTextureHandleSenteniel)
            {
                glDeleteTextures(1, &m_TextureHandle);
            }
        }

        GLuint get() const
        {
            return m_TextureHandle;
        }
    private:
        static constexpr GLuint c_EmptyTextureHandleSenteniel = static_cast<GLuint>(-1);
        GLuint m_TextureHandle;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glActiveTexture.xhtml
    inline void ActiveTexture(GLenum texture)
    {
        glActiveTexture(texture);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture(GLenum target, TextureHandle const& texture)
    {
        glBindTexture(target, texture.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // moveable RAII handle to an OpenGL texture with compile-time known type
    template<GLenum TextureType>
    class Texture {
    public:
        static constexpr GLenum type = TextureType;

        [[nodiscard]] constexpr GLuint get() const
        {
            return m_TextureHandle.get();
        }

        constexpr TextureHandle const& handle() const
        {
            return m_TextureHandle;
        }

        constexpr TextureHandle& handle()
        {
            return m_TextureHandle;
        }

    private:
        TextureHandle m_TextureHandle;
    };

    class Texture2D : public Texture<GL_TEXTURE_2D> {};
    class TextureCubemap : public Texture<GL_TEXTURE_CUBE_MAP> {};
    class Texture2DMultisample : public Texture<GL_TEXTURE_2D_MULTISAMPLE> {};

    template<typename Texture>
    inline void BindTexture(Texture const& t)
    {
        glBindTexture(t.type, t.get());
    }

    // moveable RAII handle to an OpenGL framebuffer (i.e. a render target)
    class FrameBuffer final {
    public:
        FrameBuffer()
        {
            glGenFramebuffers(1, &m_FboHandle);

            if (m_FboHandle == c_EmptyFBOSenteniel)
            {
                throw OpenGlException{"glGenFramebuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        FrameBuffer(FrameBuffer const&) = delete;

        FrameBuffer(FrameBuffer&& tmp) noexcept :
            m_FboHandle{std::exchange(tmp.m_FboHandle, c_EmptyFBOSenteniel)}
        {
        }

        FrameBuffer& operator=(FrameBuffer const&) = delete;

        FrameBuffer& operator=(FrameBuffer&& tmp) noexcept
        {
            std::swap(m_FboHandle, tmp.m_FboHandle);
            return *this;
        }

        ~FrameBuffer() noexcept
        {
            if (m_FboHandle != c_EmptyFBOSenteniel)
            {
                glDeleteFramebuffers(1, &m_FboHandle);
            }
        }

        GLuint get() const
        {
            return m_FboHandle;
        }

    private:
        static constexpr GLuint c_EmptyFBOSenteniel = static_cast<GLuint>(-1);
        GLuint m_FboHandle;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFramebuffer(GLenum target, FrameBuffer const& fb)
    {
        glBindFramebuffer(target, fb.get());
    }

    // bind to the main Window FBO for the current OpenGL context
    struct WindowFbo final {};
    static constexpr WindowFbo windowFbo{};

    inline void BindFramebuffer(GLenum target, WindowFbo)
    {
        glBindFramebuffer(target, 0);
    }

    // assign a 2D texture to the framebuffer (so that subsequent draws/reads
    // to/from the FBO use the texture)
    template<typename Texture>
    inline void FramebufferTexture2D(GLenum target, GLenum attachment, Texture const& t, GLint level)
    {
        glFramebufferTexture2D(target, attachment, t.type, t.get(), level);
    }

    // moveable RAII handle to an OpenGL render buffer
    class RenderBuffer final {
    public:
        RenderBuffer()
        {
            glGenRenderbuffers(1, &m_RenderBuffer);

            if (m_RenderBuffer == c_EmptyRenderBufferSenteniel)
            {
                throw OpenGlException{"glGenRenderBuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        RenderBuffer(RenderBuffer const&) = delete;

        RenderBuffer(RenderBuffer&& tmp) noexcept :
            m_RenderBuffer{std::exchange(tmp.m_RenderBuffer, c_EmptyRenderBufferSenteniel)}
        {
        }

        RenderBuffer& operator=(RenderBuffer const&) = delete;

        RenderBuffer& operator=(RenderBuffer&& tmp) noexcept
        {
            std::swap(m_RenderBuffer, tmp.m_RenderBuffer);
            return *this;
        }

        ~RenderBuffer() noexcept
        {
            if (m_RenderBuffer != c_EmptyRenderBufferSenteniel)
            {
                glDeleteRenderbuffers(1, &m_RenderBuffer);
            }
        }

        GLuint get() const
        {
            return m_RenderBuffer;
        }
    private:
        // khronos: glDeleteRenderBuffers: "The name zero is reserved by the GL and is silently ignored"
        static constexpr GLuint c_EmptyRenderBufferSenteniel = 0;
        GLuint m_RenderBuffer;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer(RenderBuffer& rb)
    {
        glBindRenderbuffer(GL_RENDERBUFFER, rb.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer()
    {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    inline void FramebufferRenderbuffer(GLenum target, GLenum attachment, RenderBuffer const& rb)
    {
        glFramebufferRenderbuffer(target, attachment, GL_RENDERBUFFER, rb.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glRenderbufferStorage.xhtml
    inline void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
    {
        glRenderbufferStorage(target, internalformat, width, height);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glClear.xhtml
    inline void Clear(GLbitfield mask)
    {
        glClear(mask);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArrays.xhtml
    inline void DrawArrays(GLenum mode, GLint first, GLsizei count)
    {
        glDrawArrays(mode, first, count);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArraysInstanced.xhtml
    inline void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
    {
        glDrawArraysInstanced(mode, first, count, instancecount);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribDivisor.xhtml
    template<typename Attribute>
    inline void VertexAttribDivisor(Attribute loc, GLuint divisor)
    {
        glVertexAttribDivisor(loc.get(), divisor);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawElements.xhtml
    inline void DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
    {
        glDrawElements(mode, count, type, indices);
    }

    inline void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
    {
        glClearColor(red, green, blue, alpha);
    }

    inline void ClearColor(Vec4 const& v)
    {
        ClearColor(v[0], v[1], v[2], v[3]);
    }

    inline void Viewport(GLint x, GLint y, GLsizei w, GLsizei h)
    {
        glViewport(x, y, w, h);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    inline void TexParameteri(GLenum target, GLenum pname, GLint param)
    {
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
        const void* pixels)
    {
        glTexImage2D(
            target,
            level,
            internalformat,
            width,
            height,
            border,
            format,
            type,
            pixels
        );
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    template<typename Texture>
    inline void TextureParameteri(Texture const& texture, GLenum pname, GLint param)
    {
        glTextureParameteri(texture.raw_handle(), pname, param);
    }

    inline bool IsCurrentFboComplete()
    {
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
        GLenum filter)
    {
        glBlitFramebuffer(
            srcX0,
            srcY0,
            srcX1,
            srcY1,
            dstX0,
            dstY0,
            dstX1,
            dstY1,
            mask,
            filter
        );
    }

    inline void DrawBuffer(GLenum mode)
    {
        glDrawBuffer(mode);
    }

    inline GLint GetInteger(GLenum pname)
    {
        GLint out;
        glGetIntegerv(pname, &out);
        return out;
    }

    inline GLenum GetEnum(GLenum pname)
    {
        return static_cast<GLenum>(GetInteger(pname));
    }

    inline void Enable(GLenum cap)
    {
        glEnable(cap);
    }

    inline void Disable(GLenum cap)
    {
        glDisable(cap);
    }

    inline void PixelStorei(GLenum name, GLint param)
    {
        glPixelStorei(name, param);
    }
}

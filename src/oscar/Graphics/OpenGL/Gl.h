#pragma once

#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Utils/Concepts.h>

#include <GL/glew.h>

#include <bit>
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
        OpenGlException(std::string message) : message_{std::move(message)}
        {}

        const char* what() const noexcept final { return message_.c_str(); }

    private:
        std::string message_;
    };

    // a moveable handle to an OpenGL shader
    class ShaderHandle {
    public:
        explicit ShaderHandle(GLenum type) :
            shader_handle_{glCreateShader(type)}
        {
            if (shader_handle_ == c_empty_shader_sentinel) {
                throw OpenGlException{"glCreateShader() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        ShaderHandle(const ShaderHandle&) = delete;

        ShaderHandle(ShaderHandle&& tmp) noexcept :
            shader_handle_{std::exchange(tmp.shader_handle_, c_empty_shader_sentinel)}
        {}

        ShaderHandle& operator=(const ShaderHandle&) = delete;

        ShaderHandle& operator=(ShaderHandle&& tmp) noexcept
        {
            std::swap(shader_handle_, tmp.shader_handle_);
            return *this;
        }

        ~ShaderHandle() noexcept
        {
            if (shader_handle_ != c_empty_shader_sentinel) {
                glDeleteShader(shader_handle_);
            }
        }

        GLuint get() const { return shader_handle_; }

    private:
        static constexpr GLuint c_empty_shader_sentinel = 0;
        GLuint shader_handle_;
    };

    // compile a shader from source
    void compile_from_source(const ShaderHandle&, const GLchar* src);

    // a shader of a particular type (e.g. GL_FRAGMENT_SHADER) that owns a
    // shader handle
    template<GLuint ShaderType>
    class Shader {
    public:
        static constexpr GLuint type = ShaderType;

        Shader() : shader_handle_{type} {}

        GLuint get() const { return shader_handle_.get(); }

        ShaderHandle& handle() { return shader_handle_; }
        const ShaderHandle& handle() const { return shader_handle_; }

    private:
        ShaderHandle shader_handle_;
    };

    class VertexShader : public Shader<GL_VERTEX_SHADER> {};
    class FragmentShader : public Shader<GL_FRAGMENT_SHADER> {};
    class GeometryShader : public Shader<GL_GEOMETRY_SHADER> {};

    template<typename TShader>
    inline TShader compile_from_source(const GLchar* src)
    {
        TShader rv;
        compile_from_source(rv.handle(), src);
        return rv;
    }

    // an OpenGL program (i.e. n shaders linked into one pipeline)
    class Program final {
    public:
        Program() : program_handle_{glCreateProgram()}
        {
            if (program_handle_ == c_empty_program_sentinel) {
                throw OpenGlException{"glCreateProgram() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        Program(const Program&) = delete;

        Program(Program&& tmp) noexcept :
            program_handle_{std::exchange(tmp.program_handle_, c_empty_program_sentinel)}
        {}

        Program& operator=(const Program&) = delete;

        Program& operator=(Program&& tmp) noexcept
        {
            std::swap(program_handle_, tmp.program_handle_);
            return *this;
        }

        ~Program() noexcept
        {
            if (program_handle_ != c_empty_program_sentinel) {
                glDeleteProgram(program_handle_);
            }
        }

        GLuint get() const { return program_handle_; }

    private:
        static constexpr GLuint c_empty_program_sentinel = 0;
        GLuint program_handle_;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void use_program(const Program& program)
    {
        glUseProgram(program.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void use_program()
    {
        glUseProgram(static_cast<GLuint>(0));
    }

    inline void attach_shader(Program& program, const ShaderHandle& shader)
    {
        glAttachShader(program.get(), shader.get());
    }

    template<GLuint ShaderType>
    inline void attach_shader(Program& program, const Shader<ShaderType>& shader)
    {
        glAttachShader(program.get(), shader.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glLinkProgram.xhtml
    void link_program(Program&);

    inline gl::Program create_program_from(
        const VertexShader& vertex_shader,
        const FragmentShader& fragment_shader)
    {
        gl::Program program;
        attach_shader(program, vertex_shader);
        attach_shader(program, fragment_shader);
        link_program(program);
        return program;
    }

    inline gl::Program create_program_from(
        const VertexShader& vertex_shader,
        const FragmentShader& fragment_shader,
        const GeometryShader& geometry_shader)
    {
        gl::Program program;
        attach_shader(program, vertex_shader);
        attach_shader(program, fragment_shader);
        attach_shader(program, geometry_shader);
        link_program(program);
        return program;
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    //     - throws on error
    [[nodiscard]] inline GLint get_uniform_location(
        const Program& program,
        const GLchar* uniform_name)
    {
        const GLint location = glGetUniformLocation(program.get(), uniform_name);
        if (location == -1) {
            throw OpenGlException{std::string{"glGetUniformLocation() failed: cannot get "} + uniform_name};
        }
        return location;
    }

    [[nodiscard]] inline GLint get_attribute_location(
        const Program& program,
        const GLchar* attribute_name)
    {
        const GLint location = glGetAttribLocation(program.get(), attribute_name);
        if (location == -1) {
            throw OpenGlException{std::string{"glGetAttribLocation() failed: cannot get "} + attribute_name};
        }
        return location;
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
            static constexpr size_t elements_per_location = 4;
        };
        struct mat3 final {
            static constexpr GLint size = 9;
            static constexpr GLenum type = GL_FLOAT;
            static constexpr size_t elements_per_location = 3;
        };
        struct mat4x3 final {
            static constexpr GLint size = 12;
            static constexpr GLenum type = GL_FLOAT;
            static constexpr size_t elements_per_location = 3;
        };
    }

    // a uniform shader symbol (e.g. `uniform mat4 uProjectionMatrix`) at a
    // particular location in a linked OpenGL program
    template<typename TGlsl>
    class Uniform_ {
    public:
        constexpr Uniform_(GLint _location) :
            uniform_location_{_location}
        {}

        Uniform_(const Program& program, const GLchar* uniform_name) :
            uniform_location_{get_uniform_location(program, uniform_name)}
        {}

        [[nodiscard]] constexpr GLuint get() const
        {
            return static_cast<GLuint>(uniform_location_);
        }

        [[nodiscard]] constexpr GLint geti() const
        {
            return uniform_location_;
        }

    private:
        GLint uniform_location_;
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
    inline void set_uniform(UniformFloat& uniform, GLfloat value)
    {
        glUniform1f(uniform.geti(), value);
    }

    // set the value of an `GLint` uniform in the currently bound program
    inline void set_uniform(UniformInt& uniform, GLint value)
    {
        glUniform1i(uniform.geti(), value);
    }

    // set the value of an array-like uniform `GLint`
    inline void set_uniform(const UniformInt& uniform, GLsizei num_elements, const GLint* data)
    {
        glUniform1iv(uniform.geti(), num_elements, data);
    }

    // set the value of a `vec3` uniform
    inline void set_uniform(UniformVec3& uniform, float x, float y, float z)
    {
        glUniform3f(uniform.geti(), x, y, z);
    }

    // set the value of a `vec3` uniform
    inline void set_uniform(UniformVec3& uniform, const float values[3])
    {
        glUniform3fv(uniform.geti(), 1, values);
    }

    // set the value of a `sampler2D` uniform
    inline void set_uniform(UniformSampler2D& uniform, GLint value)
    {
        glUniform1i(uniform.geti(), value);
    }

    inline void set_uniform(UniformSamplerCube& uniform, GLint value)
    {
        glUniform1i(uniform.geti(), value);
    }

    // set the value of an `sampler2DMS` uniform
    inline void set_uniform(UniformSampler2DMS& uniform, GLint value)
    {
        glUniform1i(uniform.geti(), value);
    }

    // set the value of a `bool` uniform
    inline void set_uniform(UniformBool& u, bool value)
    {
        glUniform1i(u.geti(), value);
    }

    // a uniform that points to a statically-sized array of values in the shader
    //
    // This is just a uniform that points to the first element. The utility of
    // this class is that it disambiguates overloads (so that calling code can
    // assign sequences of values to uniform arrays)
    template<typename TGlsl, size_t N>
    class UniformArray final : public Uniform_<TGlsl> {
    public:
        constexpr UniformArray(GLint location) :
            Uniform_<TGlsl>{location}
        {}

        UniformArray(const Program& program, const GLchar* uniform_name) :
            Uniform_<TGlsl>{program, uniform_name}
        {}

        [[nodiscard]] constexpr size_t size() const { return N; }
    };

    inline void set_uniform(UniformMat3& uniform, const Mat3& mat)
    {
        glUniformMatrix3fv(uniform.geti(), 1, false, value_ptr(mat));
    }

    inline void set_uniform(UniformVec4& uniform, const Vec4& vec)
    {
        glUniform4fv(uniform.geti(), 1, value_ptr(vec));
    }

    inline void set_uniform(UniformVec3& uniform, const Vec3& vec)
    {
        glUniform3fv(uniform.geti(), 1, value_ptr(vec));
    }

    inline void set_uniform(UniformVec3& uniform, std::span<const Vec3> vectors)
    {
        static_assert(sizeof(Vec3) == 3 * sizeof(GLfloat));
        glUniform3fv(uniform.geti(), static_cast<GLsizei>(vectors.size()), value_ptr(vectors.front()));
    }

    // set a uniform array of vec3s from a userspace container type (e.g. vector<Vec3>)
    template<std::ranges::contiguous_range R, size_t N>
    requires std::same_as<typename R::value_type, Vec3>
    inline void set_uniform(UniformArray<glsl::vec3, N>& uniform, R& range)
    {
        OSC_ASSERT(std::ranges::size(range) == N);
        glUniform3fv(uniform.geti(), static_cast<GLsizei>(std::ranges::size(range)), ValuePtr(*std::ranges::data(range)));
    }

    inline void set_uniform(UniformMat4& uniform, const Mat4& mat)
    {
        glUniformMatrix4fv(uniform.geti(), 1, false, value_ptr(mat));
    }

    inline void set_uniform(UniformMat4& uniform, std::span<const Mat4> matrices)
    {
        static_assert(sizeof(Mat4) == 16 * sizeof(GLfloat));
        glUniformMatrix4fv(uniform.geti(), static_cast<GLsizei>(matrices.size()), false, value_ptr(matrices.front()));
    }

    inline void set_uniform(UniformVec2& u, const Vec2& v)
    {
        glUniform2fv(u.geti(), 1, value_ptr(v));
    }

    inline void set_uniform(UniformVec2& uniform, std::span<const Vec2> vectors)
    {
        static_assert(sizeof(Vec2) == 2 * sizeof(GLfloat));

        glUniform2fv(
            uniform.geti(),
            static_cast<GLsizei>(vectors.size()),
            value_ptr(vectors.front())
        );
    }

    template<std::ranges::contiguous_range R, size_t N>
    requires std::same_as<typename R::value_type, Vec2>
    void set_uniform(UniformArray<glsl::vec2, N>& uniform, const R& range)
    {
        glUniform2fv(
            uniform.geti(),
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
            attribute_location_{location}
        {}

        Attribute(const Program& program, const GLchar* attribute_name) :
            attribute_location_{get_attribute_location(program, attribute_name)}
        {}

        [[nodiscard]] constexpr GLuint get() const
        {
            return static_cast<GLuint>(attribute_location_);
        }

        [[nodiscard]] constexpr GLint geti() const
        {
            return attribute_location_;
        }

    private:
        GLint attribute_location_;
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
    inline void vertex_attrib_pointer(
        const Attribute<TGlsl>& attribute,
        bool normalized,
        size_t stride,
        size_t offset)
    {
        static_assert(TGlsl::size <= 4 or TGlsl::type == GL_FLOAT);

        const GLboolean normgl = normalized ? GL_TRUE : GL_FALSE;
        const GLsizei stridegl = static_cast<GLsizei>(stride);

        if constexpr (TGlsl::size <= 4) {
            glVertexAttribPointer(
                attribute.get(),
                TGlsl::size,
                SourceType,
                normgl,
                stridegl,
                std::bit_cast<void*>(offset)
            );
        }
        else if constexpr (SourceType == GL_FLOAT) {
            for (size_t i = 0; i < TGlsl::size / TGlsl::elements_per_location; ++i) {
                glVertexAttribPointer(
                    attribute.get() + static_cast<GLuint>(i),
                    TGlsl::elements_per_location,
                    SourceType,
                    normgl,
                    stridegl,
                    std::bit_cast<void*>(offset + (i * TGlsl::elements_per_location * sizeof(float)))
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
    inline void enable_vertex_attrib_array(const Attribute<TGlsl>& attribute)
    {
        static_assert(TGlsl::size <= 4 or TGlsl::type == GL_FLOAT);

        if constexpr (TGlsl::size <= 4) {
            glEnableVertexAttribArray(attribute.get());
        }
        else if constexpr (TGlsl::type == GL_FLOAT) {
            for (size_t i = 0; i < TGlsl::size / TGlsl::elements_per_location; ++i) {
                glEnableVertexAttribArray(attribute.get() + static_cast<GLuint>(i));
            }
        }

        // else: not supported: see static_assert above
    }

    template<typename TGlsl>
    inline void disable_vertex_attrib_array(const Attribute<TGlsl>& loc)
    {
        static_assert(TGlsl::size <= 4 or TGlsl::type == GL_FLOAT);

        if constexpr (TGlsl::size <= 4) {
            glDisableVertexAttribArray(loc.get());
        }
        else if constexpr (TGlsl::type == GL_FLOAT) {
            for (size_t i = 0; i < TGlsl::size / TGlsl::elements_per_location; ++i) {
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
    inline void vertex_attrib_divisor(const Attribute<TGlsl>& loc, GLuint divisor)
    {
        static_assert(TGlsl::size <= 4 or TGlsl::type == GL_FLOAT);

        if constexpr (TGlsl::size <= 4) {
            glVertexAttribDivisor(loc.get(), divisor);
        }
        else if constexpr (TGlsl::type == GL_FLOAT) {
            for (size_t i = 0; i < TGlsl::size / TGlsl::elements_per_location; ++i) {
                glVertexAttribDivisor(loc.get() + static_cast<GLuint>(i), divisor);
            }
        }
    }

    // a moveable handle to an OpenGL buffer (e.g. GL_ARRAY_BUFFER)
    class BufferHandle {
    public:
        BufferHandle()
        {
            glGenBuffers(1, &buffer_handle_);

            if (buffer_handle_ == c_empty_buffer_handle_sentinel) {
                throw OpenGlException{"glGenBuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        BufferHandle(const BufferHandle&) = delete;

        BufferHandle(BufferHandle&& tmp) noexcept :
            buffer_handle_{std::exchange(tmp.buffer_handle_, c_empty_buffer_handle_sentinel)}
        {}

        BufferHandle& operator=(const BufferHandle&) = delete;

        BufferHandle& operator=(BufferHandle&& tmp) noexcept
        {
            std::swap(buffer_handle_, tmp.buffer_handle_);
            return *this;
        }

        ~BufferHandle() noexcept
        {
            if (buffer_handle_ != c_empty_buffer_handle_sentinel) {
                glDeleteBuffers(1, &buffer_handle_);
            }
        }

        GLuint get() const { return buffer_handle_; }

    private:
        static constexpr GLuint c_empty_buffer_handle_sentinel = static_cast<GLuint>(-1);
        GLuint buffer_handle_;
    };

    // a buffer handle that is locked against a particular type (e.g. GL_ELEMENT_ARRAY_BUFFER)
    template<GLenum TBuffer>
    class TypedBufferHandle : public BufferHandle {
        using BufferHandle::BufferHandle;

    public:
        static constexpr GLenum BufferType = TBuffer;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    inline void bind_buffer(GLenum target, const BufferHandle& handle)
    {
        glBindBuffer(target, handle.get());
    }

    template<GLenum TBuffer>
    inline void bind_buffer(const TypedBufferHandle<TBuffer>& handle)
    {
        glBindBuffer(TBuffer, handle.get());
    }

    template<GLenum TBuffer>
    inline void unbind_buffer(const TypedBufferHandle<TBuffer>&)
    {
        glBindBuffer(TBuffer, 0);
    }

    inline void buffer_data(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
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

        template<std::ranges::contiguous_range R>
        void assign(const R& range)
        {
            bind_buffer(*this);
            buffer_data(BufferType, sizeof(T) * std::ranges::size(range), std::ranges::data(range), Usage);
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
    inline void bind_buffer(const Buffer& buffer)
    {
        glBindBuffer(Buffer::BufferType, buffer.get());
    }

    // returns an OpenGL enum that describes the provided (integral) type
    // argument, so that the index type to an element-based drawcall can
    // be computed at compile-time
    template<std::unsigned_integral T>
    inline constexpr GLenum index_type();

    template<>
    inline constexpr GLenum index_type<uint8_t>() { return GL_UNSIGNED_BYTE; }

    template<>
    inline constexpr GLenum index_type<uint16_t>() { return GL_UNSIGNED_SHORT; }

    template<>
    inline constexpr GLenum index_type<uint32_t>() { return GL_UNSIGNED_INT; }

    // utility overload of index_type specifically for EBOs (the most common
    // use-case in downstream code)
    template<typename T>
    inline constexpr GLenum index_type(const gl::ElementArrayBuffer<T>&)
    {
        return index_type<T>();
    }

    // a handle to an OpenGL VAO with RAII semantics for glGenVertexArrays etc.
    class VertexArray final {
    public:
        VertexArray()
        {
            glGenVertexArrays(1, &vao_handle_);
            if (vao_handle_ == c_empty_vao_sentinel) {
                throw OpenGlException{"glGenVertexArrays() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        VertexArray(const VertexArray&) = delete;

        VertexArray(VertexArray&& tmp) noexcept :
            vao_handle_{std::exchange(tmp.vao_handle_, c_empty_vao_sentinel)}
        {}

        VertexArray& operator=(const VertexArray&) = delete;

        VertexArray& operator=(VertexArray&& tmp) noexcept
        {
            std::swap(vao_handle_, tmp.vao_handle_);
            return *this;
        }

        ~VertexArray() noexcept
        {
            if (vao_handle_ != c_empty_vao_sentinel) {
                glDeleteVertexArrays(1, &vao_handle_);
            }
        }

        GLuint get() const { return vao_handle_; }

    private:
        static constexpr GLuint c_empty_vao_sentinel = std::numeric_limits<GLuint>::max();
        GLuint vao_handle_;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void bind_vertex_array(const VertexArray& vao)
    {
        glBindVertexArray(vao.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void bind_vertex_array()
    {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    // moveable RAII handle to an OpenGL texture (e.g. GL_TEXTURE_2D)
    class TextureHandle {
    public:
        TextureHandle()
        {
            glGenTextures(1, &texture_handle_);
            if (texture_handle_ == c_empty_texture_handle_sentinel) {
                throw OpenGlException{"glGenTextures() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        TextureHandle(const TextureHandle&) = delete;

        TextureHandle(TextureHandle&& tmp) noexcept :
            texture_handle_{std::exchange(tmp.texture_handle_, c_empty_texture_handle_sentinel)}
        {}

        TextureHandle& operator=(const TextureHandle&) = delete;

        TextureHandle& operator=(TextureHandle&& tmp) noexcept
        {
            std::swap(texture_handle_, tmp.texture_handle_);
            return *this;
        }

        ~TextureHandle() noexcept
        {
            if (texture_handle_ != c_empty_texture_handle_sentinel) {
                glDeleteTextures(1, &texture_handle_);
            }
        }

        GLuint get() const { return texture_handle_; }
    private:
        static constexpr GLuint c_empty_texture_handle_sentinel = static_cast<GLuint>(-1);
        GLuint texture_handle_;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glActiveTexture.xhtml
    inline void active_texture(GLenum texture)
    {
        glActiveTexture(texture);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void bind_texture(GLenum target, const TextureHandle& texture)
    {
        glBindTexture(target, texture.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void bind_texture()
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
            return texture_handle_.get();
        }

        constexpr const TextureHandle& handle() const
        {
            return texture_handle_;
        }

        constexpr TextureHandle& handle()
        {
            return texture_handle_;
        }

    private:
        TextureHandle texture_handle_;
    };

    class Texture2D : public Texture<GL_TEXTURE_2D> {};
    class TextureCubemap : public Texture<GL_TEXTURE_CUBE_MAP> {};
    class Texture2DMultisample : public Texture<GL_TEXTURE_2D_MULTISAMPLE> {};

    template<typename Texture>
    inline void bind_texture(const Texture& texture)
    {
        glBindTexture(texture.type, texture.get());
    }

    // moveable RAII handle to an OpenGL framebuffer (i.e. a render target)
    class FrameBuffer final {
    public:
        FrameBuffer()
        {
            glGenFramebuffers(1, &fbo_handle_);
            if (fbo_handle_ == c_empty_fbo_sentinel) {
                throw OpenGlException{"glGenFramebuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        FrameBuffer(const FrameBuffer&) = delete;

        FrameBuffer(FrameBuffer&& tmp) noexcept :
            fbo_handle_{std::exchange(tmp.fbo_handle_, c_empty_fbo_sentinel)}
        {}

        FrameBuffer& operator=(const FrameBuffer&) = delete;

        FrameBuffer& operator=(FrameBuffer&& tmp) noexcept
        {
            std::swap(fbo_handle_, tmp.fbo_handle_);
            return *this;
        }

        ~FrameBuffer() noexcept
        {
            if (fbo_handle_ != c_empty_fbo_sentinel) {
                glDeleteFramebuffers(1, &fbo_handle_);
            }
        }

        GLuint get() const { return fbo_handle_; }

    private:
        static constexpr GLuint c_empty_fbo_sentinel = static_cast<GLuint>(-1);
        GLuint fbo_handle_;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void bind_framebuffer(GLenum target, const FrameBuffer& framebuffer)
    {
        glBindFramebuffer(target, framebuffer.get());
    }

    // bind to the main Window FBO for the current OpenGL context
    struct WindowFramebuffer final {};
    static constexpr WindowFramebuffer window_framebuffer{};

    inline void bind_framebuffer(GLenum target, WindowFramebuffer)
    {
        glBindFramebuffer(target, 0);
    }

    // assign a 2D texture to the framebuffer (so that subsequent draws/reads
    // to/from the FBO use the texture)
    template<typename Texture>
    inline void framebuffer_texture2D(GLenum target, GLenum attachment, const Texture& texture, GLint level)
    {
        glFramebufferTexture2D(target, attachment, texture.type, texture.get(), level);
    }

    // moveable RAII handle to an OpenGL render buffer
    class RenderBuffer final {
    public:
        RenderBuffer()
        {
            glGenRenderbuffers(1, &rbo_handle_);
            if (rbo_handle_ == c_empty_rbo_sentinel) {
                throw OpenGlException{"glGenRenderBuffers() failed: this could mean that your GPU/system is out of memory, or that your OpenGL driver is invalid in some way"};
            }
        }

        RenderBuffer(const RenderBuffer&) = delete;

        RenderBuffer(RenderBuffer&& tmp) noexcept :
            rbo_handle_{std::exchange(tmp.rbo_handle_, c_empty_rbo_sentinel)}
        {
        }

        RenderBuffer& operator=(const RenderBuffer&) = delete;

        RenderBuffer& operator=(RenderBuffer&& tmp) noexcept
        {
            std::swap(rbo_handle_, tmp.rbo_handle_);
            return *this;
        }

        ~RenderBuffer() noexcept
        {
            if (rbo_handle_ != c_empty_rbo_sentinel) {
                glDeleteRenderbuffers(1, &rbo_handle_);
            }
        }

        GLuint get() const { return rbo_handle_; }
    private:
        // khronos: glDeleteRenderBuffers: "The uniform_name zero is reserved by the GL and is silently ignored"
        static constexpr GLuint c_empty_rbo_sentinel = 0;
        GLuint rbo_handle_;
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void bind_renderbuffer(RenderBuffer& renderbuffer)
    {
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void bind_renderbuffer()
    {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    inline void framebuffer_renderbuffer(GLenum target, GLenum attachment, const RenderBuffer& renderbuffer)
    {
        glFramebufferRenderbuffer(target, attachment, GL_RENDERBUFFER, renderbuffer.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glRenderbufferStorage.xhtml
    inline void renderbuffer_storage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
    {
        glRenderbufferStorage(target, internalformat, width, height);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glClear.xhtml
    inline void clear(GLbitfield mask)
    {
        glClear(mask);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArrays.xhtml
    inline void draw_arrays(GLenum mode, GLint first, GLsizei count)
    {
        glDrawArrays(mode, first, count);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArraysInstanced.xhtml
    inline void draw_arrays_instanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
    {
        glDrawArraysInstanced(mode, first, count, instancecount);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribDivisor.xhtml
    template<typename Attribute>
    inline void vertex_attrib_divisor(Attribute loc, GLuint divisor)
    {
        glVertexAttribDivisor(loc.get(), divisor);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawElements.xhtml
    inline void draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices)
    {
        glDrawElements(mode, count, type, indices);
    }

    inline void clear_color(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
    {
        glClearColor(red, green, blue, alpha);
    }

    inline void clear_color(const Vec4& v)
    {
        clear_color(v[0], v[1], v[2], v[3]);
    }

    inline void viewport(GLint x, GLint y, GLsizei w, GLsizei h)
    {
        glViewport(x, y, w, h);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    inline void tex_parameter_i(GLenum target, GLenum pname, GLint param)
    {
        glTexParameteri(target, pname, param);
    }

    inline void tex_image2D(
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
    inline void texture_parameter_i(const Texture& texture, GLenum pname, GLint param)
    {
        glTextureParameteri(texture.raw_handle(), pname, param);
    }

    inline bool is_currently_bound_fbo_complete()
    {
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBlitFramebuffer.xhtml
    inline void blit_framebuffer(
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

    inline void draw_buffer(GLenum mode)
    {
        glDrawBuffer(mode);
    }

    inline GLint get_integer(GLenum pname)
    {
        GLint out;
        glGetIntegerv(pname, &out);
        return out;
    }

    inline GLenum get_enum(GLenum pname)
    {
        return static_cast<GLenum>(get_integer(pname));
    }

    inline void enable(GLenum cap)
    {
        glEnable(cap);
    }

    inline void disable(GLenum cap)
    {
        glDisable(cap);
    }

    inline void pixel_store_i(GLenum name, GLint param)
    {
        glPixelStorei(name, param);
    }
}

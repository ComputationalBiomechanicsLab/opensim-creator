#pragma once

#include <oscar/Graphics/Detail/CPUDataType.hpp>
#include <oscar/Graphics/OpenGL/Gl.hpp>

namespace osc::detail
{
    template<CPUDataType>
    struct CPUDataTypeOpenGLTraits;

    template<>
    struct CPUDataTypeOpenGLTraits<CPUDataType::UnsignedByte> {
        static inline constexpr GLenum opengl_data_type = GL_UNSIGNED_BYTE;
    };

    template<>
    struct CPUDataTypeOpenGLTraits<CPUDataType::Float> {
        static inline constexpr GLenum opengl_data_type = GL_FLOAT;
    };

    template<>
    struct CPUDataTypeOpenGLTraits<CPUDataType::UnsignedInt24_8> {
        static inline constexpr GLenum opengl_data_type = GL_UNSIGNED_INT_24_8;
    };

    template<>
    struct CPUDataTypeOpenGLTraits<CPUDataType::HalfFloat> {
        static inline constexpr GLenum opengl_data_type = GL_HALF_FLOAT;
    };
}

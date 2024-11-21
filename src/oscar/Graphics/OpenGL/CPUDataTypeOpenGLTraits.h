#pragma once

#include <oscar/Graphics/Detail/CPUDataType.h>
#include <oscar/Graphics/OpenGL/Gl.h>

namespace osc::detail
{
    template<CPUDataType>
    struct CPUDataTypeOpenGLTraits;

    template<>
    struct CPUDataTypeOpenGLTraits<CPUDataType::UnsignedByte> {
        static constexpr GLenum opengl_data_type = GL_UNSIGNED_BYTE;
    };

    template<>
    struct CPUDataTypeOpenGLTraits<CPUDataType::Float> {
        static constexpr GLenum opengl_data_type = GL_FLOAT;
    };

    template<>
    struct CPUDataTypeOpenGLTraits<CPUDataType::UnsignedInt24_8> {
        static constexpr GLenum opengl_data_type = GL_UNSIGNED_INT_24_8;
    };

    template<>
    struct CPUDataTypeOpenGLTraits<CPUDataType::HalfFloat> {
        static constexpr GLenum opengl_data_type = GL_HALF_FLOAT;
    };
}

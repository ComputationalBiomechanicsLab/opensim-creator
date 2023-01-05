#pragma once

#include <cstdint>
#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // data type of a material-assignable property parsed from the shader code
    enum class ShaderType : int32_t {
        Float = 0,
        Vec2,
        Vec3,
        Vec4,
        Mat3,
        Mat4,
        Int,
        Bool,
        Sampler2D,
        Unknown,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, ShaderType);
}
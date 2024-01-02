#pragma once

#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // data type of a material-assignable property parsed from the shader code
    enum class ShaderPropertyType {
        Float,
        Vec2,
        Vec3,
        Vec4,
        Mat3,
        Mat4,
        Int,
        Bool,
        Sampler2D,
        SamplerCube,
        Unknown,

        NUM_OPTIONS,
    };

    std::ostream& operator<<(std::ostream&, ShaderPropertyType);
}

#pragma once

#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // data type of a material-assignable property parsed from the shader code
    enum class ShaderPropertyType {
        Float,
        Vector2,
        Vector3,
        Vector4,
        Matrix3x3,
        Matrix4x4,
        Int,
        Bool,
        Sampler2D,
        SamplerCube,
        Unknown,
        NUM_OPTIONS,
    };

    std::ostream& operator<<(std::ostream&, ShaderPropertyType);
}

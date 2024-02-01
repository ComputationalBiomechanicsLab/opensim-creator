#pragma once

#include <oscar/Graphics/ShaderPropertyType.hpp>
#include <oscar/Utils/NonTypelist.hpp>

namespace osc::detail
{
    using ShaderPropertyTypeList = NonTypelist<ShaderPropertyType,
        ShaderPropertyType::Float,
        ShaderPropertyType::Vec2,
        ShaderPropertyType::Vec3,
        ShaderPropertyType::Vec4,
        ShaderPropertyType::Mat3,
        ShaderPropertyType::Mat4,
        ShaderPropertyType::Int,
        ShaderPropertyType::Bool,
        ShaderPropertyType::Sampler2D,
        ShaderPropertyType::SamplerCube,
        ShaderPropertyType::Unknown
    >;
}

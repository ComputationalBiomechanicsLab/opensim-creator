#pragma once

#include <liboscar/Graphics/ShaderPropertyType.h>
#include <liboscar/Utils/EnumHelpers.h>

namespace osc::detail
{
    using ShaderPropertyTypeList = OptionList<ShaderPropertyType,
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

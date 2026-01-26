#pragma once

#include <liboscar/graphics/shader_property_type.h>
#include <liboscar/utilities/enum_helpers.h>

namespace osc::detail
{
    using ShaderPropertyTypeList = OptionList<ShaderPropertyType,
        ShaderPropertyType::Float,
        ShaderPropertyType::Vector2,
        ShaderPropertyType::Vector3,
        ShaderPropertyType::Vector4,
        ShaderPropertyType::Matrix3x3,
        ShaderPropertyType::Matrix4x4,
        ShaderPropertyType::Int,
        ShaderPropertyType::Bool,
        ShaderPropertyType::Sampler2D,
        ShaderPropertyType::SamplerCube,
        ShaderPropertyType::Unknown
    >;
}

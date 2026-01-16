#pragma once

#include <liboscar/graphics/shader_property_type.h>

#include <string_view>

namespace osc::detail
{
    template<ShaderPropertyType>
    struct ShaderPropertyTypeTraits;

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Float> {
        static constexpr std::string_view name = "Float";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vector2> {
        static constexpr std::string_view name = "Vector2";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vector3> {
        static constexpr std::string_view name = "Vector3";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vector4> {
        static constexpr std::string_view name = "Vector4";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Matrix3x3> {
        static constexpr std::string_view name = "Matrix3x3";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Matrix4x4> {
        static constexpr std::string_view name = "Matrix4x4";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Int> {
        static constexpr std::string_view name = "Int";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Bool> {
        static constexpr std::string_view name = "Bool";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Sampler2D> {
        static constexpr std::string_view name = "Sampler2D";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::SamplerCube> {
        static constexpr std::string_view name = "SamplerCube";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Unknown> {
        static constexpr std::string_view name = "Unknown";
    };
}

#pragma once

#include <liboscar/Graphics/ShaderPropertyType.h>

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
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Mat3> {
        static constexpr std::string_view name = "Mat3";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Mat4> {
        static constexpr std::string_view name = "Mat4";
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

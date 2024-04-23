#pragma once

#include <oscar/Graphics/ShaderPropertyType.h>

#include <string_view>

namespace osc::detail
{
    template<ShaderPropertyType>
    struct ShaderPropertyTypeTraits;

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Float> {
        static inline constexpr std::string_view name = "Float";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vec2> {
        static inline constexpr std::string_view name = "Vec2";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vec3> {
        static inline constexpr std::string_view name = "Vec3";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vec4> {
        static inline constexpr std::string_view name = "Vec4";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Mat3> {
        static inline constexpr std::string_view name = "Mat3";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Mat4> {
        static inline constexpr std::string_view name = "Mat4";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Int> {
        static inline constexpr std::string_view name = "Int";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Bool> {
        static inline constexpr std::string_view name = "Bool";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Sampler2D> {
        static inline constexpr std::string_view name = "Sampler2D";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::SamplerCube> {
        static inline constexpr std::string_view name = "SamplerCube";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Unknown> {
        static inline constexpr std::string_view name = "Unknown";
    };
}

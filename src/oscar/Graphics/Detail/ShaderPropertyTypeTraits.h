#pragma once

#include <oscar/Graphics/ShaderPropertyType.h>
#include <oscar/Utils/CStringView.h>

namespace osc::detail
{
    template<ShaderPropertyType>
    struct ShaderPropertyTypeTraits;

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Float> {
        static inline constexpr CStringView name = "Float";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vec2> {
        static inline constexpr CStringView name = "Vec2";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vec3> {
        static inline constexpr CStringView name = "Vec3";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Vec4> {
        static inline constexpr CStringView name = "Vec4";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Mat3> {
        static inline constexpr CStringView name = "Mat3";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Mat4> {
        static inline constexpr CStringView name = "Mat4";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Int> {
        static inline constexpr CStringView name = "Int";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Bool> {
        static inline constexpr CStringView name = "Bool";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Sampler2D> {
        static inline constexpr CStringView name = "Sampler2D";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::SamplerCube> {
        static inline constexpr CStringView name = "SamplerCube";
    };

    template<>
    struct ShaderPropertyTypeTraits<ShaderPropertyType::Unknown> {
        static inline constexpr CStringView name = "Unknown";
    };
}

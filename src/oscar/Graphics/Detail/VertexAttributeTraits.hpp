#pragma once

#include <oscar/Graphics/Detail/ShaderLocations.hpp>
#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>

namespace osc::detail
{
    template<VertexAttribute>
    struct VertexAttributeTraits;

    template<>
    struct VertexAttributeTraits<VertexAttribute::Position> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x3;
        static inline constexpr int shader_location = shader_locations::aPos;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Normal> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x3;
        static inline constexpr int shader_location = shader_locations::aNormal;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Tangent> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x4;
        static inline constexpr int shader_location = shader_locations::aTangent;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Color> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x4;
        static inline constexpr int shader_location = shader_locations::aColor;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::TexCoord0> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x2;
        static inline constexpr int shader_location = shader_locations::aTexCoord;
    };
}

#pragma once

#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>

namespace osc::detail
{
    template<VertexAttribute>
    struct VertexAttributeTraits;

    template<>
    struct VertexAttributeTraits<VertexAttribute::Position> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x3;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Normal> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x3;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Tangent> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x4;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Color> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x4;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::TexCoord0> final {
        static inline constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x2;
    };
}

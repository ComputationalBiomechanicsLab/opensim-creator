#pragma once

#include <liboscar/graphics/vertex_attribute.h>
#include <liboscar/graphics/vertex_attribute_format.h>

namespace osc::detail
{
    template<VertexAttribute>
    struct VertexAttributeTraits;

    template<>
    struct VertexAttributeTraits<VertexAttribute::Position> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x3;
        static constexpr int shader_location = 0;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Normal> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x3;
        static constexpr int shader_location = 2;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Tangent> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x4;
        static constexpr int shader_location = 4;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Color> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x4;
        static constexpr int shader_location = 3;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::TexCoord0> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x2;
        static constexpr int shader_location = 1;
    };
}

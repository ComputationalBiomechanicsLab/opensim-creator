#pragma once

#include <oscar/Graphics/Detail/ShaderLocations.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

namespace osc::detail
{
    template<VertexAttribute>
    struct VertexAttributeTraits;

    template<>
    struct VertexAttributeTraits<VertexAttribute::Position> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x3;
        static constexpr int shader_location = shader_locations::aPos;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Normal> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x3;
        static constexpr int shader_location = shader_locations::aNormal;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Tangent> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x4;
        static constexpr int shader_location = shader_locations::aTangent;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::Color> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x4;
        static constexpr int shader_location = shader_locations::aColor;
    };

    template<>
    struct VertexAttributeTraits<VertexAttribute::TexCoord0> final {
        static constexpr VertexAttributeFormat default_format = VertexAttributeFormat::Float32x2;
        static constexpr int shader_location = shader_locations::aTexCoord;
    };
}

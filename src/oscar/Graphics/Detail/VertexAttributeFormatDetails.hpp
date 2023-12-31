#pragma once

#include <oscar/Graphics/Detail/Unorm8.hpp>
#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>
#include <oscar/Maths/Vec.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <cstddef>

namespace osc::detail
{
    struct VertexAttributeFormatDetails final {
        friend bool operator==(VertexAttributeFormatDetails const&, VertexAttributeFormatDetails const&) = default;

        size_t stride() const { return numComponents * sizeOfComponent; }

        size_t numComponents;
        size_t sizeOfComponent;
    };

    constexpr VertexAttributeFormatDetails GetDetails(VertexAttributeFormat f)
    {
        static_assert(NumOptions<VertexAttributeFormat>() == 4);

        switch (f) {
        case VertexAttributeFormat::Float32x2: return {2, sizeof(float)};
        case VertexAttributeFormat::Float32x3: return {3, sizeof(float)};
        case VertexAttributeFormat::Float32x4: return {4, sizeof(float)};
        case VertexAttributeFormat::Unorm8x4:  return {4, sizeof(char)};
        default:                               return {4, sizeof(float)};
        }
    }

    constexpr VertexAttributeFormat DefaultFormat(VertexAttribute attr)
    {
        static_assert(NumOptions<VertexAttribute>() == 5);

        switch (attr) {
        case VertexAttribute::Position:  return VertexAttributeFormat::Float32x3;
        case VertexAttribute::Normal:    return VertexAttributeFormat::Float32x3;
        case VertexAttribute::Tangent:   return VertexAttributeFormat::Float32x4;
        case VertexAttribute::Color:     return VertexAttributeFormat::Float32x4;
        case VertexAttribute::TexCoord0: return VertexAttributeFormat::Float32x2;
        default:                         return VertexAttributeFormat::Float32x4;
        }
    }

    template<VertexAttributeFormat>
    struct VertexAttributeFormatCPU;

    template<>
    struct VertexAttributeFormatCPU<VertexAttributeFormat::Float32x2> final {
        using type = Vec2;
    };

    template<>
    struct VertexAttributeFormatCPU<VertexAttributeFormat::Float32x3> final {
        using type = Vec3;
    };

    template<>
    struct VertexAttributeFormatCPU<VertexAttributeFormat::Float32x4> final {
        using type = Vec4;
    };

    template<>
    struct VertexAttributeFormatCPU<VertexAttributeFormat::Unorm8x4> final {
        using type = Vec<4, Unorm8>;
    };

    template<VertexAttributeFormat Format>
    using VertexAttributeFormatCPUType = VertexAttributeFormatCPU<Format>::type;
}

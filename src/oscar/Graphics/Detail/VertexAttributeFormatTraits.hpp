#pragma once

#include <oscar/Graphics/Detail/Unorm8.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>
#include <oscar/Maths/Vec.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>

#include <cstddef>

namespace osc::detail
{
    template<VertexAttributeFormat>
    struct VertexAttributeFormatTraits;

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x2> final {
        using type = Vec2;
        using component_type = type::value_type;
        static inline constexpr size_t num_components = type::length();
        static inline constexpr size_t component_size = sizeof(component_type);
        static inline constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x3> final {
        using type = Vec3;
        using component_type = type::value_type;
        static inline constexpr size_t num_components = type::length();
        static inline constexpr size_t component_size = sizeof(component_type);
        static inline constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x4> final {
        using type = Vec4;
        using component_type = type::value_type;
        static inline constexpr size_t num_components = type::length();
        static inline constexpr size_t component_size = sizeof(component_type);
        static inline constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Unorm8x4> final {
        using type = Vec<4, Unorm8>;
        using component_type = type::value_type;
        static inline constexpr size_t num_components = type::length();
        static inline constexpr size_t component_size = sizeof(component_type);
        static inline constexpr size_t stride = num_components * component_size;
    };
}

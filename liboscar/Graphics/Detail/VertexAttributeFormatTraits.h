#pragma once

#include <liboscar/Graphics/Snorm8.h>
#include <liboscar/Graphics/Unorm8.h>
#include <liboscar/Graphics/VertexAttributeFormat.h>
#include <liboscar/Maths/Vec.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Maths/Vec4.h>

#include <cstddef>
#include <tuple>

namespace osc::detail
{
    template<VertexAttributeFormat>
    struct VertexAttributeFormatTraits;

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x2> final {
        using type = Vec2;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x3> final {
        using type = Vec3;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x4> final {
        using type = Vec4;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Unorm8x4> final {
        using type = Vec<4, Unorm8>;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Snorm8x4> final {
        using type = Vec<4, Snorm8>;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };
}

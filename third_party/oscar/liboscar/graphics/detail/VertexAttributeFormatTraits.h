#pragma once

#include <liboscar/graphics/Snorm8.h>
#include <liboscar/graphics/Unorm8.h>
#include <liboscar/graphics/VertexAttributeFormat.h>
#include <liboscar/maths/Vector.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/maths/Vector4.h>

#include <cstddef>
#include <tuple>

namespace osc::detail
{
    template<VertexAttributeFormat>
    struct VertexAttributeFormatTraits;

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x2> final {
        using type = Vector2;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x3> final {
        using type = Vector3;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Float32x4> final {
        using type = Vector4;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Unorm8x4> final {
        using type = Vector<Unorm8, 4>;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Snorm8x4> final {
        using type = Vector<Snorm8, 4>;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };
}

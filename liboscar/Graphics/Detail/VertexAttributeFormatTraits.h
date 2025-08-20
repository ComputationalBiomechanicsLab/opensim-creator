#pragma once

#include <liboscar/Graphics/Snorm8.h>
#include <liboscar/Graphics/Unorm8.h>
#include <liboscar/Graphics/VertexAttributeFormat.h>
#include <liboscar/Maths/Vector.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Maths/Vector4.h>

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
        using type = Vector<4, Unorm8>;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };

    template<>
    struct VertexAttributeFormatTraits<VertexAttributeFormat::Snorm8x4> final {
        using type = Vector<4, Snorm8>;
        using component_type = type::value_type;
        static constexpr size_t num_components = std::tuple_size_v<type>;
        static constexpr size_t component_size = sizeof(component_type);
        static constexpr size_t stride = num_components * component_size;
    };
}

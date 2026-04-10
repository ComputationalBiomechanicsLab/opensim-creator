#pragma once

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/variant/variant_type.h>

namespace osc
{
    template<VariantType>
    struct VariantTypeTraits;

    template<>
    struct VariantTypeTraits<VariantType::None> {
        static inline constexpr CStringView name = "NoneType";
    };

    template<>
    struct VariantTypeTraits<VariantType::Bool> {
        static inline constexpr CStringView name = "bool";
    };

    template<>
    struct VariantTypeTraits<VariantType::Color> {
        static inline constexpr CStringView name = "Color";
    };

    template<>
    struct VariantTypeTraits<VariantType::Float> {
        static inline constexpr CStringView name = "float";
    };

    template<>
    struct VariantTypeTraits<VariantType::Int> {
        static inline constexpr CStringView name = "int";
    };

    template<>
    struct VariantTypeTraits<VariantType::String> {
        static inline constexpr CStringView name = "String";
    };

    template<>
    struct VariantTypeTraits<VariantType::StringName> {
        static inline constexpr CStringView name = "StringName";
    };

    template<>
    struct VariantTypeTraits<VariantType::Vector2> {
        static inline constexpr CStringView name = "Vector2";
    };

    template<>
    struct VariantTypeTraits<VariantType::Vector3> {
        static inline constexpr CStringView name = "Vector3";
    };

    template<>
    struct VariantTypeTraits<VariantType::VariantArray> {
        static inline constexpr CStringView name = "VariantArray";
    };
}

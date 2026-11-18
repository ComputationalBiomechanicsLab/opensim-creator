#pragma once

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/variant/variant_type.h>

namespace osc
{
    template<VariantType>
    struct VariantTypeTraits;

    template<>
    struct VariantTypeTraits<VariantType::None> {
        static constexpr CStringView name = "NoneType";
    };

    template<>
    struct VariantTypeTraits<VariantType::Bool> {
        static constexpr CStringView name = "bool";
    };

    template<>
    struct VariantTypeTraits<VariantType::Color> {
        static constexpr CStringView name = "Color";
    };

    template<>
    struct VariantTypeTraits<VariantType::Float> {
        static constexpr CStringView name = "float";
    };

    template<>
    struct VariantTypeTraits<VariantType::Int> {
        static constexpr CStringView name = "int";
    };

    template<>
    struct VariantTypeTraits<VariantType::String> {
        static constexpr CStringView name = "String";
    };

    template<>
    struct VariantTypeTraits<VariantType::StringName> {
        static constexpr CStringView name = "StringName";
    };

    template<>
    struct VariantTypeTraits<VariantType::Vector2> {
        static constexpr CStringView name = "Vector2";
    };

    template<>
    struct VariantTypeTraits<VariantType::Vector3> {
        static constexpr CStringView name = "Vector3";
    };

    template<>
    struct VariantTypeTraits<VariantType::VariantArray> {
        static constexpr CStringView name = "VariantArray";
    };
}

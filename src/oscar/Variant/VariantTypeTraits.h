#pragma once

#include <oscar/Utils/CStringView.h>
#include <oscar/Variant/VariantType.h>

namespace osc
{
    template<VariantType>
    struct VariantTypeTraits;

    template<>
    struct VariantTypeTraits<VariantType::Nil> {
        static inline constexpr CStringView name = "Nil";
    };

    template<>
    struct VariantTypeTraits<VariantType::Bool> {
        static inline constexpr CStringView name = "Bool";
    };

    template<>
    struct VariantTypeTraits<VariantType::Color> {
        static inline constexpr CStringView name = "Color";
    };

    template<>
    struct VariantTypeTraits<VariantType::Float> {
        static inline constexpr CStringView name = "Float";
    };

    template<>
    struct VariantTypeTraits<VariantType::Int> {
        static inline constexpr CStringView name = "Int";
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
    struct VariantTypeTraits<VariantType::Vec3> {
        static inline constexpr CStringView name = "Vec3";
    };
}

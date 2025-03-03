#pragma once

#include <liboscar/Graphics/ShaderPropertyType.h>

#include <concepts>
#include <span>
#include <utility>

namespace osc::detail
{
    // Satisfied if `T` has the correct "shape" for a `MaterialValueTraits` specialization.
    template<typename T, typename BaseValueType>
    concept MaterialValueTraitsLike = requires (T v) {
        T::constructor_assertions(std::declval<std::span<const BaseValueType>>());
        { T::shader_property_type(std::declval<std::span<const BaseValueType>>()) } -> std::same_as<ShaderPropertyType>;
    };
}

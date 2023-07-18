#pragma once

#include <cstddef>
#include <type_traits>
#include <variant>

namespace osc
{
    // returns the index of the given variant type (impl)
    template<typename Variant, typename T, size_t I = 0>
    [[nodiscard]] constexpr size_t VariantIndexImpl() noexcept
    {
        if constexpr (I >= std::variant_size_v<Variant>)
        {
            return std::variant_size_v<Variant>;
        }
        else if constexpr (std::is_same_v<std::variant_alternative_t<I, Variant>, T>)
        {
            return I;
        }
        else
        {
            return VariantIndexImpl<Variant, T, I + 1>();
        }
    }

    // returns the index of the given variant type
    template<typename Variant, typename T>
    [[nodiscard]] constexpr size_t VariantIndex() noexcept
    {
        return VariantIndexImpl<Variant, T>();
    }

    template<typename... Ts>
    struct Overload : Ts... {
        using Ts::operator()...;
    };
    template<typename... Ts> Overload(Ts...) -> Overload<Ts...>;
}

#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace osc
{
    template<typename TEnum>
    concept DenselyPackedOptionsEnum = requires(TEnum v) {
        requires std::is_enum_v<TEnum>;
        TEnum::NUM_OPTIONS;
    };

    template<DenselyPackedOptionsEnum TEnum>
    constexpr size_t NumOptions()
    {
        return static_cast<size_t>(TEnum::NUM_OPTIONS);
    }

    template<DenselyPackedOptionsEnum TEnum, TEnum... TEnumOptions>
    struct OptionList {
        static_assert(sizeof...(TEnumOptions) == NumOptions<TEnum>());
    };

    template<typename TEnum>
    concept FlagsEnum = requires(TEnum v) {
        requires std::is_enum_v<TEnum>;
        TEnum::NUM_FLAGS;
    };

    template<FlagsEnum TEnum>
    constexpr size_t NumFlags()
    {
        return static_cast<size_t>(TEnum::NUM_FLAGS);
    }
}

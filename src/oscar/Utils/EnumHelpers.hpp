#pragma once

#include <cstddef>
#include <type_traits>

namespace osc
{
    template<typename TEnum>
    [[nodiscard]] constexpr size_t NumOptions() noexcept
    {
        static_assert(std::is_enum_v<TEnum>);
        return static_cast<size_t>(TEnum::NUM_OPTIONS);
    }
}

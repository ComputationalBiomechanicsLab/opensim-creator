#pragma once

#include <type_traits>

namespace osc::cpp23
{
    template<class Enum>
    constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
        requires std::is_enum_v<Enum>
    {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }
}

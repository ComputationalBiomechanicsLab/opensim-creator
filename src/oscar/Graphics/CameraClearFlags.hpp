#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace osc
{
    enum class CameraClearFlags : uint32_t {
        Nothing = 0u,
        SolidColor = 1u << 0u,
        Depth = 1u << 1u,

        All = SolidColor | Depth,
        Default = SolidColor,
    };

    constexpr CameraClearFlags operator|(CameraClearFlags a, CameraClearFlags b) noexcept
    {
        using T = std::underlying_type_t<CameraClearFlags>;
        return static_cast<CameraClearFlags>(static_cast<T>(a) | static_cast<T>(b));
    }

    constexpr bool operator&(CameraClearFlags a, CameraClearFlags b) noexcept
    {
        using T = std::underlying_type_t<CameraClearFlags>;
        return static_cast<T>(a) & static_cast<T>(b);
    }
}

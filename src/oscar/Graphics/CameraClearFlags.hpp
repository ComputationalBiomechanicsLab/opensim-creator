#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

#include <cstdint>

namespace osc
{
    enum class CameraClearFlags : uint32_t {
        Nothing    = 0u,
        SolidColor = 1u << 0u,
        Depth      = 1u << 1u,

        All = SolidColor | Depth,
        Default = SolidColor | Depth,
    };

    constexpr CameraClearFlags operator|(CameraClearFlags a, CameraClearFlags b)
    {
        return static_cast<CameraClearFlags>(osc::to_underlying(a) | osc::to_underlying(b));
    }

    constexpr bool operator&(CameraClearFlags a, CameraClearFlags b)
    {
        return osc::to_underlying(a) & osc::to_underlying(b);
    }
}

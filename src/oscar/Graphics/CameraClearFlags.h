#pragma once

#include <oscar/Shims/Cpp23/utility.h>

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

    constexpr CameraClearFlags operator|(CameraClearFlags lhs, CameraClearFlags rhs)
    {
        return static_cast<CameraClearFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr bool operator&(CameraClearFlags lhs, CameraClearFlags rhs)
    {
        return cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs);
    }
}

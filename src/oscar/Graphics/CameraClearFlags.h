#pragma once

#include <oscar/Shims/Cpp23/utility.h>

#include <cstdint>

namespace osc
{
    enum class CameraClearFlags : uint32_t {
        Nothing    = 0,
        SolidColor = 1<<0,
        Depth      = 1<<1,

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

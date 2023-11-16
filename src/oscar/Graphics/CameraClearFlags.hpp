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

    constexpr CameraClearFlags operator|(CameraClearFlags lhs, CameraClearFlags rhs)
    {
        return static_cast<CameraClearFlags>(osc::to_underlying(lhs) | osc::to_underlying(rhs));
    }

    constexpr bool operator&(CameraClearFlags lhs, CameraClearFlags rhs)
    {
        return osc::to_underlying(lhs) & osc::to_underlying(rhs);
    }
}

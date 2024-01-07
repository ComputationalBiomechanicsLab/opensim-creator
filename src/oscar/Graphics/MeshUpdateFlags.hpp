#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

namespace osc
{
    enum class MeshUpdateFlags {
        Default               = 0,
        DontValidateIndices   = 1<<0,  // also requires DontRecalculateBounds, because recalculating the bounds implicitly validates indices
        DontRecalculateBounds = 1<<1,
    };

    constexpr MeshUpdateFlags operator|(MeshUpdateFlags lhs, MeshUpdateFlags rhs)
    {
        return static_cast<MeshUpdateFlags>(osc::to_underlying(lhs) | osc::to_underlying(rhs));
    }

    constexpr bool operator&(MeshUpdateFlags lhs, MeshUpdateFlags rhs)
    {
        return (osc::to_underlying(lhs) & osc::to_underlying(rhs)) != 0;
    }
}

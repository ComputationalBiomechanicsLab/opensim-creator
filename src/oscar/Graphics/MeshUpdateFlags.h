#pragma once

#include <oscar/Shims/Cpp23/utility.h>

namespace osc
{
    enum class MeshUpdateFlags {
        Default               = 0,
        DontValidateIndices   = 1<<0,  // also requires `DontRecalculateBounds`, because recalculating the bounds implicitly validates indices
        DontRecalculateBounds = 1<<1,
    };

    constexpr MeshUpdateFlags operator|(MeshUpdateFlags lhs, MeshUpdateFlags rhs)
    {
        return static_cast<MeshUpdateFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr bool operator&(MeshUpdateFlags lhs, MeshUpdateFlags rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }
}

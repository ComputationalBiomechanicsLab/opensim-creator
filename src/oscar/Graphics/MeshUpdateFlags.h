#pragma once

#include <oscar/Shims/Cpp23/utility.h>

namespace osc
{
    enum class MeshUpdateFlags {
        Default               = 0,

        // using this flag also requires using `DontRecalculateBounds`, because
        // recalculating the bounds will implicitly validate indices "for free"
        DontValidateIndices   = 1<<0,
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

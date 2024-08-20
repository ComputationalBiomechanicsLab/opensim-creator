#pragma once

#include <oscar/Utils/Flags.h>

namespace osc
{
    enum class MeshUpdateFlag {
        None                  = 0,

        // using this flag also requires using `DontRecalculateBounds`, because
        // recalculating the bounds will implicitly validate indices "for free"
        DontValidateIndices   = 1<<0,
        DontRecalculateBounds = 1<<1,

        Default = None,
    };

    using MeshUpdateFlags = Flags<MeshUpdateFlag>;
}

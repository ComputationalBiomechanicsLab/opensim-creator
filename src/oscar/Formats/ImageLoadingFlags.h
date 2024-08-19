#pragma once

#include <oscar/Utils/Flags.h>

namespace osc
{
    enum class ImageLoadingFlag {
        None = 0,

        // BEWARE: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but can cause surprising behavior if the pixels represent vectors
        //
        // therefore, if you are flipping (e.g.) normal maps, you may *also* need
        // to flip the pixel content appropriately (e.g. if RGB represents XYZ then
        // you'll need to negate each G)
        FlipVertically = 1<<0,
    };
    using ImageLoadingFlags = Flags<ImageLoadingFlag>;
}

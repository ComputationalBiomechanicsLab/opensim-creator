#pragma once

namespace osc
{
    // flags for loading images
    using ImageFlags = int;
    enum ImageFlags_ {
        ImageFlags_None = 0,

        // BEWARE: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but can cause surprising behavior if the pixels represent vectors
        //
        // therefore, if you are flipping (e.g.) normal maps, you may *also* need
        // to flip the pixel content appropriately (e.g. if RGB represents XYZ then
        // you'll need to negate each G)
        ImageFlags_FlipVertically = 1<<1,
    };
}
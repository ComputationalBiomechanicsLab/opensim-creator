#pragma once

#include <liboscar/utils/Flags.h>

namespace osc
{
    // Represents a flag that affects how an image is loaded from disk.
    enum class ImageLoadingFlag {
        None                            = 0,

        // Makes the image loading implementation flip the image vertically.
        //
        // By default, the image-loading implementation will load image data according
        // to the underlying data format (e.g. PNG, JPEG) and then adjust it to follow
        // liboscar's internal format (right-handed, bottom-to-top). This flag causes
        // the implementation to flip the final adjusted image such that, the returned
        // image is flipped vertically.
        //
        // BEWARE: This will flip pixels vertically (in Y) but leaves the pixel's components
        //         (e.g. RGB) untouched. That's fine if the pixels represent colors, but can
        //         cause surprising behavior if the pixels represent vectors (e.g. normal maps).
        //
        //         Therefore, if you are flipping (e.g.) normal maps, you may *also* need to flip
        //         the pixel content appropriately. If RGB represents XYZ vectors then you might
        //         need to negate each G.
        FlipVertically                  = 1<<0,

        // Makes the image loading implementation handle the image's components as-if they were
        // spatial vectors.
        //
        // By default, the image-loading implementation will load image data according to the
        // underlying data format (e.g. PNG, JPEG) and then adjust it to follow liboscar's internal
        // format (right-handed, bottom-to-top). The adjustment may require internally flipping the
        // image (e.g. because PNG's are encoded top-to-bottom), which is problematic if the pixels
        // encode vectors because the flipping operation mirrors the coordinate space that the pixels
        // are encoded in. To fix that, the implementation needs to flip the green (Y) component of
        // each pixel in the image. It shouldn't perform this fix if the pixels encode colors, though,
        // which is why this flag is necessary.
        //
        // NOTE: This assumes that the encoded vectors are right-handed in the first place, because
        //       that is how liboscar defines tangent space. If the vectors are left-handed (e.g.
        //       because you baked the texture in Unreal Engine or 3ds Max) then you should _also_
        //       manually negate the green components in shaders, or memory.
        //
        // - see: http://wiki.polycount.com/wiki/Normal_Map_Technical_Details
        TreatComponentsAsSpatialVectors = 1<<1,
    };
    using ImageLoadingFlags = Flags<ImageLoadingFlag>;
}

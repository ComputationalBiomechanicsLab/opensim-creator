#pragma once

#include <oscar/Formats/ImageLoadingFlags.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Texture2D.hpp>

#include <iosfwd>

namespace osc { class ResourceStream; }

namespace osc
{
    // loads the given image stream into a Texture2D
    //
    // throws if the image data isn't representable as a GPU texture (e.g. because it has
    // an incorrect number of color channels)
    Texture2D LoadTexture2DFromImage(
        ResourceStream&&,
        ColorSpace,
        ImageLoadingFlags = ImageLoadingFlags::None
    );

    void WriteToPNG(
        Texture2D const&,
        std::ostream&
    );
}

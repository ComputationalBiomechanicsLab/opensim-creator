#pragma once

#include <oscar/Formats/ImageLoadingFlags.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Utils/Concepts.h>

#include <iosfwd>
#include <utility>

namespace osc
{
    // loads the given (named) image stream into a `Texture2D`
    //
    // throws if the image data isn't representable as a GPU texture (e.g. because it has
    // an incorrect number of color channels)
    Texture2D load_texture2D_from_image(
        std::istream&,
        std::string_view name,
        ColorSpace,
        ImageLoadingFlags = ImageLoadingFlags::None
    );

    template<NamedInputStream Stream>
    Texture2D load_texture2D_from_image(
        Stream&& stream,
        ColorSpace color_space,
        ImageLoadingFlags flags = ImageLoadingFlags::None)
    {
        return load_texture2D_from_image(
            std::forward<Stream>(stream),
            stream.name(),
            color_space,
            flags
        );
    }

    void write_to_png(
        const Texture2D&,
        std::ostream&
    );
}

#pragma once

#include <liboscar/Concepts/NamedInputStream.h>
#include <liboscar/Formats/ImageLoadingFlags.h>
#include <liboscar/Graphics/ColorSpace.h>
#include <liboscar/Graphics/Texture2D.h>

#include <iosfwd>
#include <utility>

namespace osc
{
    // loads the given (named) image stream into a `Texture2D`
    //
    // throws if the image data isn't representable as a GPU texture (e.g. because it has
    // an incorrect number of components)
    Texture2D load_texture2D_from_image(
        std::istream&,
        std::string_view input_name,
        ColorSpace,
        ImageLoadingFlags = {}
    );

    template<NamedInputStream Stream>
    Texture2D load_texture2D_from_image(
        Stream&& stream,
        ColorSpace color_space,
        ImageLoadingFlags flags = {})
    {
        return load_texture2D_from_image(
            std::forward<Stream>(stream),
            stream.name(),
            color_space,
            flags
        );
    }

    void write_to_png(const Texture2D&, std::ostream&);
    void write_to_jpeg(const Texture2D&, std::ostream&, float quality = 0.9f);
}

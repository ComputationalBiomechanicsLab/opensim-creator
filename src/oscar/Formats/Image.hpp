#pragma once

#include <oscar/Formats/ImageLoadingFlags.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Utils/Concepts.hpp>

#include <iosfwd>
#include <utility>

namespace osc
{
    // loads the given (named) image stream into a Texture2D
    //
    // throws if the image data isn't representable as a GPU texture (e.g. because it has
    // an incorrect number of color channels)
    Texture2D LoadTexture2DFromImage(
        std::istream&,
        std::string_view name,
        ColorSpace,
        ImageLoadingFlags = ImageLoadingFlags::None
    );

    template<NamedInputStream Stream>
    Texture2D LoadTexture2DFromImage(
        Stream&& stream,
        ColorSpace colorSpace,
        ImageLoadingFlags flags = ImageLoadingFlags::None)
    {
        return LoadTexture2DFromImage(
            std::forward<Stream>(stream),
            stream.name(),
            colorSpace,
            flags
        );
    }

    void WriteToPNG(
        Texture2D const&,
        std::ostream&
    );
}

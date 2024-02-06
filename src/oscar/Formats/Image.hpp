#pragma once

#include <oscar/Formats/ImageLoadingFlags.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Texture2D.hpp>

#include <filesystem>
#include <iosfwd>
#include <string_view>

namespace osc
{
    // loads the given image stream into a Texture2D
    //
    // throws if the image data isn't representable as a GPU texture (e.g. because it has
    // an incorrect number of color channels)
    Texture2D LoadTexture2DFromImage(
        std::istream&,
        std::string_view nameForErrorMessagesExtensionChecksEtc,
        ColorSpace,
        ImageLoadingFlags = ImageLoadingFlags::None
    );

    Texture2D LoadTexture2DFromImage(
        std::filesystem::path const&,
        ColorSpace,
        ImageLoadingFlags = ImageLoadingFlags::None
    );

    void WriteToPNG(
        Texture2D const&,
        std::ostream&
    );
}

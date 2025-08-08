#pragma once

#include <liboscar/Concepts/NamedInputStream.h>
#include <liboscar/Formats/ImageLoadingFlags.h>
#include <liboscar/Graphics/ColorSpace.h>
#include <liboscar/Graphics/Texture2D.h>

#include <iosfwd>
#include <utility>

namespace osc
{
    class Image final {
    public:
        // Read the given (named) image stream into a `Texture2D`.
        //
        // Throws if the image data isn't representable as a GPU texture (e.g. because it has
        // an incorrect number of components).
        static Texture2D read_into_texture(
            std::istream&,
            std::string_view input_name,
            ColorSpace,
            ImageLoadingFlags = {}
        );

        template<NamedInputStream Stream>
        static Texture2D read_into_texture(
            Stream&& stream,
            ColorSpace color_space,
            ImageLoadingFlags flags = {})
        {
            return read_into_texture(
                std::forward<Stream>(stream),
                stream.name(),
                color_space,
                flags
            );
        }
    };

    class PNG final {
    public:
        static void write(std::ostream&, const Texture2D&);
    };

    class JPEG final {
    public:
        static void write(std::ostream&, const Texture2D&, float quality = 0.9f);
    };
}

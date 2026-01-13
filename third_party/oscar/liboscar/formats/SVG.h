#pragma once

#include <liboscar/graphics/Texture2D.h>

#include <iosfwd>

namespace osc
{
    class SVG final {
    public:
        static Texture2D read_into_texture(
            std::istream&,
            float scale = 1.0f,
            float device_pixel_ratio = 1.0f
        );
    };

}

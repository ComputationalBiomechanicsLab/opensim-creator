#pragma once

#include <liboscar/Graphics/Texture2D.h>

#include <iosfwd>

namespace osc
{
    Texture2D load_texture2D_from_svg(
        std::istream&,
        float scale = 1.0f,
        float device_pixel_ratio = 1.0f
    );
}

#pragma once

#include <oscar/Graphics/Texture2D.h>

#include <iosfwd>

namespace osc
{
    Texture2D readSVGIntoTexture(
        std::istream&,
        float scale = 1.0f
    );
}

#pragma once

#include <oscar/Graphics/Texture2D.h>

#include <iosfwd>

namespace osc
{
    Texture2D ReadSVGIntoTexture(
        std::istream&,
        float scale = 1.0f
    );
}

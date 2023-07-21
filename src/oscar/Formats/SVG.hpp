#pragma once

#include "oscar/Graphics/Texture2D.hpp"

#include <filesystem>
#include <iosfwd>

namespace osc
{
    Texture2D ReadSVGIntoTexture(
        std::istream&,
        float scale = 1.0f
    );

    Texture2D LoadTextureFromSVGFile(
        std::filesystem::path const&,
        float scale = 1.0f
    );
}

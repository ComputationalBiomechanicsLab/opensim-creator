#pragma once

#include "src/Graphics/Texture2D.hpp"

#include <filesystem>

namespace osc
{
    Texture2D LoadTextureFromSVGFile(std::filesystem::path const&, float scale = 1.0f);
}
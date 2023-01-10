#pragma once

#include "src/Graphics/Texture2D.hpp"

#include <filesystem>
#include <string_view>

namespace osc
{
    osc::Texture2D LoadTextureFromSVGFile(std::filesystem::path const&, float scale = 1.0f);
    osc::Texture2D LoadTextureFromSVGResource(std::string_view, float scale = 1.0f);
}
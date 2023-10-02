#pragma once

#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Platform/ScreenshotAnnotation.hpp>

#include <vector>

namespace osc
{
    struct Screenshot final {
        Texture2D image;
        std::vector<ScreenshotAnnotation> annotations;
    };
}

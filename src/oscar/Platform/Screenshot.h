#pragma once

#include <oscar/Graphics/Texture2D.h>
#include <oscar/Platform/ScreenshotAnnotation.h>

#include <vector>

namespace osc
{
    struct Screenshot final {
        Texture2D image;
        std::vector<ScreenshotAnnotation> annotations;
    };
}

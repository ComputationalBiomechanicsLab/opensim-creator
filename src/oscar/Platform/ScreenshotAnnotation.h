#pragma once

#include <oscar/Maths/Rect.h>

#include <string>

namespace osc
{
    struct ScreenshotAnnotation final {
        std::string label;
        Rect rect;
    };
}

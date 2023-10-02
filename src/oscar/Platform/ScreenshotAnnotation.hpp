#pragma once

#include <oscar/Maths/Rect.hpp>

#include <string>

namespace osc
{
    struct ScreenshotAnnotation final {
        std::string label;
        Rect rect;
    };
}

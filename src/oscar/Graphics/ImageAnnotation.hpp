#pragma once

#include "oscar/Maths/Rect.hpp"

#include <string>

namespace osc
{
    struct ImageAnnotation final {
        std::string label;
        Rect rect;
    };
}
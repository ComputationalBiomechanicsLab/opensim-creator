#pragma once

#include <oscar/Maths/Rect.h>

#include <string>
#include <utility>

namespace osc
{
    class ScreenshotAnnotation final {
    public:
        ScreenshotAnnotation(std::string label, Rect rect) :
            label_{std::move(label)},
            rect_{rect}
        {}

        const std::string& label() const { return label_; }
        const Rect& rect() const { return rect_; }
    private:
        std::string label_;
        Rect rect_;
    };
}

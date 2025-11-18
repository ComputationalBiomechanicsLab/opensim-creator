#pragma once

#include <liboscar/Maths/Rect.h>

#include <string>
#include <utility>

namespace osc
{
    class ScreenshotAnnotation final {
    public:
        explicit ScreenshotAnnotation(
            std::string label,
            const Rect& screenspace_rect) :
            label_{std::move(label)},
            rect_{screenspace_rect}
        {}

        const std::string& label() const { return label_; }

        // Returns the bounding rectangle of the annotation in screenspace in
        // device-independent pixels.
        const Rect& rect() const { return rect_; }

    private:
        std::string label_;
        Rect rect_;
    };
}

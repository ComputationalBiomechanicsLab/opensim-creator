#pragma once

#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Platform/ScreenshotAnnotation.h>

#include <span>
#include <utility>
#include <vector>

namespace osc
{
    class Screenshot final {
    public:
        explicit Screenshot(
            Texture2D texture,
            std::vector<ScreenshotAnnotation> annotations) :

            texture_{std::move(texture)},
            annotations_{std::move(annotations)}
        {}

        const Texture2D& texture() const { return texture_; }
        Vec2i dimensions() const { return texture_.dimensions(); }
        Vec2 device_independent_dimensions() const { return texture_.device_independent_dimensions(); }
        std::span<const ScreenshotAnnotation> annotations() const { return annotations_; }

    private:
        Texture2D texture_;
        std::vector<ScreenshotAnnotation> annotations_;
    };
}

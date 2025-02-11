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
        Screenshot(
            Texture2D image,
            std::vector<ScreenshotAnnotation> annotations) :

            image_{std::move(image)},
            annotations_{std::move(annotations)}
        {}

        const Texture2D& image() const { return image_; }
        Vec2i dimensions() const { return image_.dimensions(); }
        Vec2 device_independent_dimensions() const { return image_.device_independent_dimensions(); }
        std::span<const ScreenshotAnnotation> annotations() const { return annotations_; }

    private:
        Texture2D image_;
        std::vector<ScreenshotAnnotation> annotations_;
    };
}

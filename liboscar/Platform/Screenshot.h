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

        // Returns a texture that represents the content of the screenshot.
        const Texture2D& texture() const { return texture_; }

        // Returns the dimensions of the screenshot in physical pixels.
        Vec2i pixel_dimensions() const { return texture_.pixel_dimensions(); }

        // Returns the dimensions of the screenshot in device-independent pixels.
        Vec2 dimensions() const { return texture_.dimensions(); }

        // Returns a sequence of annotations (metadata) associated with the screenshot.
        std::span<const ScreenshotAnnotation> annotations() const { return annotations_; }

    private:
        Texture2D texture_;
        std::vector<ScreenshotAnnotation> annotations_;
    };
}

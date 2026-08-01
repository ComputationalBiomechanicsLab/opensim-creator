#pragma once

#include <liboscar/graphics/clear_flags.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/rect.h>

#include <optional>

namespace osc
{
    /// Represents the top-level configuration options for a single render pass.
    struct RenderPassConfig final {
        friend bool operator==(const RenderPassConfig&, const RenderPassConfig&) = default;

        /// Where the renderer should rasterize its pixels in the render
        /// target. The rectangle is defined in screen space, which:
        ///
        /// - Is measured in device-independent pixels
        /// - Starts in the bottom-left corner
        /// - Ends in the top-right corner
        ///
        /// `std::nullopt` makes the renderer use the full extents of the
        /// render target.
        std::optional<Rect> viewport_rect{};

        /// The scissor rectangle, which tells the renderer to only clear and/or
        /// render fragments (pixels) that occur within the given sub-rectangle in the
        /// render target. The rectangle is defined in screen space, which:
        ///
        /// - Is measured in device-independent pixels
        /// - Starts in the bottom-left corner
        /// - Ends in the top-right corner
        ///
        /// `std::nullopt` implies that the camera should clear (if applicable) the entire
        /// output, followed by writing output fragments to the full extents of `pixel_rect`
        /// with no scissoring.
        std::optional<Rect> scissor_rect{};

        /// The color that the render target's color attachments should be cleared
        /// with - if `clear_flags` indicates the attachments should be cleared.
        Color clear_color = Color::clear();

        /// Flags that tell the renderer if (and what) it should clear
        /// before rendering.
        ClearFlags clear_flags = ClearFlag::Default;
    };
}

#pragma once

#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/color_render_buffer_format.h>
#include <liboscar/graphics/depth_stencil_render_buffer_format.h>
#include <liboscar/graphics/texture_dimensionality.h>
#include <liboscar/maths/vector2.h>

#include <iosfwd>

namespace osc
{
    struct RenderTextureParams final {
        friend bool operator==(const RenderTextureParams&, const RenderTextureParams&) = default;

        // The dimensions of the texture in physical pixels.
        Vector2i pixel_dimensions = {1, 1};

        // The ratio of the resolution of the texture in physical pixels
        // to the resolution of it in device-independent pixels.
        float device_pixel_ratio = 1.0f;

        TextureDimensionality dimensionality = TextureDimensionality::Tex2D;
        AntiAliasingLevel anti_aliasing_level = AntiAliasingLevel{1};
        ColorRenderBufferFormat color_format = ColorRenderBufferFormat::Default;
        DepthStencilRenderBufferFormat depth_stencil_format = DepthStencilRenderBufferFormat::Default;
    };

    std::ostream& operator<<(std::ostream&, const RenderTextureParams&);
}

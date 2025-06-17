#pragma once

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/ColorRenderBufferFormat.h>
#include <liboscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <liboscar/Graphics/TextureDimensionality.h>
#include <liboscar/Maths/Vec2.h>

#include <iosfwd>

namespace osc
{
    struct RenderTextureParams final {
        friend bool operator==(const RenderTextureParams&, const RenderTextureParams&) = default;

        // The dimensions of the texture in physical pixels.
        Vec2i pixel_dimensions = {1, 1};

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

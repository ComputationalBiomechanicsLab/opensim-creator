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

        Vec2i dimensions = {1, 1};
        TextureDimensionality dimensionality = TextureDimensionality::Tex2D;
        AntiAliasingLevel anti_aliasing_level = AntiAliasingLevel{1};
        ColorRenderBufferFormat color_format = ColorRenderBufferFormat::Default;
        DepthStencilRenderBufferFormat depth_stencil_format = DepthStencilRenderBufferFormat::Default;
    };

    std::ostream& operator<<(std::ostream&, const RenderTextureParams&);
}

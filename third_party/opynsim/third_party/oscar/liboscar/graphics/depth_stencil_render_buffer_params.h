#pragma once

#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/depth_stencil_render_buffer_format.h>
#include <liboscar/graphics/texture_dimensionality.h>
#include <liboscar/maths/vector2.h>

namespace osc
{
    struct DepthStencilRenderBufferParams final {

        friend bool operator==(const DepthStencilRenderBufferParams&, const DepthStencilRenderBufferParams&) = default;

        Vector2i pixel_dimensions = {1, 1};
        TextureDimensionality dimensionality = TextureDimensionality::Tex2D;
        AntiAliasingLevel anti_aliasing_level = AntiAliasingLevel{1};
        DepthStencilRenderBufferFormat format = DepthStencilRenderBufferFormat::Default;
    };
}

#pragma once

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <liboscar/Graphics/TextureDimensionality.h>
#include <liboscar/Maths/Vec2.h>

namespace osc
{
    struct DepthStencilRenderBufferParams final {

        friend bool operator==(const DepthStencilRenderBufferParams&, const DepthStencilRenderBufferParams&) = default;

        Vec2i dimensions = {1, 1};
        TextureDimensionality dimensionality = TextureDimensionality::Tex2D;
        AntiAliasingLevel anti_aliasing_level = AntiAliasingLevel{1};
        DepthStencilRenderBufferFormat format = DepthStencilRenderBufferFormat::Default;
    };
}

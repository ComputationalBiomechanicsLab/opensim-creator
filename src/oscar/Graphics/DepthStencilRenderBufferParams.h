#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>

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

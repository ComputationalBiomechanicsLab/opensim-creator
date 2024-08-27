#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/ColorRenderBufferFormat.h>
#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <oscar/Graphics/RenderBufferReadWrite.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>

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
        RenderBufferReadWrite read_write = RenderBufferReadWrite::Default;
    };

    std::ostream& operator<<(std::ostream&, const RenderTextureParams&);
}

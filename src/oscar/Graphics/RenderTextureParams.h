#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
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
        RenderTextureFormat color_format = RenderTextureFormat::ARGB32;
        DepthStencilFormat depth_stencil_format = DepthStencilFormat::Default;
        RenderTextureReadWrite read_write = RenderTextureReadWrite::Default;
    };

    std::ostream& operator<<(std::ostream&, const RenderTextureParams&);
}

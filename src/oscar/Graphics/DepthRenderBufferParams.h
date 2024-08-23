#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>

namespace osc
{
    struct DepthRenderBufferParams final {

        friend bool operator==(const DepthRenderBufferParams&, const DepthRenderBufferParams&) = default;

        Vec2i dimensions = {1, 1};
        TextureDimensionality dimensionality = TextureDimensionality::Tex2D;
        AntiAliasingLevel anti_aliasing_level = AntiAliasingLevel{1};
        RenderTextureReadWrite read_write = RenderTextureReadWrite::Default;
        DepthStencilFormat depth_format = DepthStencilFormat::D24_UNorm_S8_UInt;
    };
}

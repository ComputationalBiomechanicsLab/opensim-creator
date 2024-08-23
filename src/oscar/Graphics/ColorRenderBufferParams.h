#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>

namespace osc
{
    struct ColorRenderBufferParams final {
        friend bool operator==(const ColorRenderBufferParams&, const ColorRenderBufferParams&) = default;

        Vec2i dimensions = {1, 1};
        TextureDimensionality dimensionality = TextureDimensionality::Tex2D;
        AntiAliasingLevel anti_aliasing_level = AntiAliasingLevel{1};
        RenderTextureReadWrite read_write = RenderTextureReadWrite::Default;
        RenderTextureFormat color_format = RenderTextureFormat::Default;
    };
}

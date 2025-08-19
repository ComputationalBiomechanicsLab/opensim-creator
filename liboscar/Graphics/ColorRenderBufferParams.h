#pragma once

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/ColorRenderBufferFormat.h>
#include <liboscar/Graphics/TextureDimensionality.h>
#include <liboscar/Maths/Vector2.h>

namespace osc
{
    struct ColorRenderBufferParams final {
        friend bool operator==(const ColorRenderBufferParams&, const ColorRenderBufferParams&) = default;

        Vector2i pixel_dimensions = {1, 1};
        TextureDimensionality dimensionality = TextureDimensionality::Tex2D;
        AntiAliasingLevel anti_aliasing_level = AntiAliasingLevel{1};
        ColorRenderBufferFormat format = ColorRenderBufferFormat::Default;
    };
}

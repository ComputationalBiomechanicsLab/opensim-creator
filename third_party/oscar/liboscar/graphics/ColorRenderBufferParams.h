#pragma once

#include <liboscar/graphics/AntiAliasingLevel.h>
#include <liboscar/graphics/ColorRenderBufferFormat.h>
#include <liboscar/graphics/TextureDimensionality.h>
#include <liboscar/maths/Vector2.h>

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

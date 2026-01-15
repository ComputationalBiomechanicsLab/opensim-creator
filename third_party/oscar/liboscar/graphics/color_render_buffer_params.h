#pragma once

#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/color_render_buffer_format.h>
#include <liboscar/graphics/texture_dimensionality.h>
#include <liboscar/maths/vector2.h>

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

#pragma once

#include <oscar/Graphics/Rgba.h>

#include <optional>
#include <string>
#include <string_view>

namespace osc
{
    using Color = Rgba<float>;

    // returns the linearized version of a sRGB component value
    float to_linear_colorspace(float srgb_component_value);

    // returns the sRGB version of a linearized component value
    float to_srgb_colorspace(float linear_component_value);

    // returns the linear version of a (presumed to be) sRGB color
    Color to_linear_colorspace(const Color&);

    // returns a color that is the (presumed to be) linear color with the sRGB gamma curve applied
    Color to_srgb_colorspace(const Color&);

    // returns the color as a hexadecimal string in the format "#rrggbbaa", as
    // commonly-used in web applications, configuration files, etc.
    //
    // - HDR values are clamped to LDR (they can't fit in this format)
    // - examples:
    //   - red --> "#ff0000ff"
    //   - green --> "#00ff00ff"
    //   - blue --> "#0000ffff"
    //   - black --> "#000000ff"
    //   - clear --> "#00000000"
    //   - etc.
    std::string to_html_string_rgba(const Color&);
    std::optional<Color> try_parse_html_color_string(std::string_view);

    // returns a color that is the result of converting `color` to HSLA multiplying
    // its luminance (L) by `factor`, and converting it back to RGBA
    Color multiply_luminance(const Color& color, float factor);
}

#pragma once

#include <oscar/Graphics/Color32.h>
#include <oscar/Graphics/ColorHSLA.h>
#include <oscar/Graphics/Rgba.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <tuple>

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

    // returns a color that is clamped to the low-dynamic range (LDR, i.e. [0, 1])
    Color clamp_to_ldr(const Color&);

    // returns the HSL(A) equivalent of the given (RGBA) color
    ColorHSLA to_hsla_color(const Color&);

    // returns the color (RGBA) equivalent of the given HSL color
    Color to_color(const ColorHSLA&);

    // returns a Vec4 version of a Color
    inline Vec4 to_vec4(const Color& color)
    {
        return Vec4{color};
    }

    // linearly interpolates all components of `a` and `b` by the interpolant `t`
    //
    // `t` is clamped to [0.0f, 1.0f]. When `t` is 0, returns `a`. When `t` is 1, returns `b`
    Color lerp(const Color& a, const Color& b, float t);

    // float-/double-based inputs assume normalized color range (i.e. 0 to 1)
    Color32 to_color32(const Color&);
    Color32 to_color32(const Vec4&);
    Color32 to_color32(float, float, float, float);
    Color32 to_color32(uint32_t);  // R at MSB
    Color to_color(Color32);

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

    // returns a color that is the result of converting `color` to HSLA,
    // multiplying its luminance (L) by `factor`, and converting it back to RGBA
    Color multiply_luminance(const Color& color, float factor);
}

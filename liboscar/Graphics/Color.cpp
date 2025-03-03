#include "Color.h"

#include <liboscar/Graphics/Color32.h>
#include <liboscar/Graphics/ColorHSLA.h>
#include <liboscar/Graphics/Unorm8.h>
#include <liboscar/Maths/CommonFunctions.h>
#include <liboscar/Utils/StringHelpers.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

// the sRGB <--> linear relationship is commonly simplified to:
//
// - linear = sRGB ^ 2.2
// - sRGB = linear ^ (1.0/2.2)
//
// but the actual equation is a little more nuanced, and is explained here:
//
// - https://en.wikipedia.org/wiki/SRGB
//
// and this implementation is effectively copied from:
//
// - https://stackoverflow.com/questions/61138110/what-is-the-correct-gamma-correction-function
// - https://registry.khronos.org/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt
//
// (because I am a lazy bastard)
float osc::to_linear_colorspace(float srgb_component_value)
{
    if (srgb_component_value <= 0.04045f) {
        return srgb_component_value / 12.92f;
    }
    else {
        return pow((srgb_component_value + 0.055f) / 1.055f, 2.4f);
    }
}

float osc::to_srgb_colorspace(float linear_component_value)
{
    if (linear_component_value <= 0.0031308f) {
        return linear_component_value * 12.92f;
    }
    else {
        return pow(linear_component_value, 1.0f/2.4f)*1.055f - 0.055f;
    }
}

Color osc::to_linear_colorspace(const Color& color)
{
    return {
        to_linear_colorspace(color.r),
        to_linear_colorspace(color.g),
        to_linear_colorspace(color.b),
        color.a,
    };
}

Color osc::to_srgb_colorspace(const Color& color)
{
    return {
        to_srgb_colorspace(color.r),
        to_srgb_colorspace(color.g),
        to_srgb_colorspace(color.b),
        color.a,
    };
}

std::string osc::to_html_string_rgba(const Color& color)
{
    std::string rv;
    rv.reserve(9);
    rv.push_back('#');
    for (auto component : Color32{color}) {
        auto [nibble_1, nibble_2] = to_hex_chars(static_cast<uint8_t>(component));
        rv.push_back(nibble_1);
        rv.push_back(nibble_2);
    }
    return rv;
}

std::optional<Color> osc::try_parse_html_color_string(std::string_view str)
{
    if (str.empty()) {
        return std::nullopt;  // it's empty
    }
    if (str.front() != '#') {
        return std::nullopt;  // incorrect first character (e.g. should be "#ff0000ff")
    }

    const std::string_view content = str.substr(1);
    if (content.size() == 6) {
        // RGB hex string (e.g. "ffaa88")
        Color rv = Color::black();
        for (size_t i = 0; i < 3; ++i) {
            if (auto b = try_parse_hex_chars_as_byte(content[2*i], content[2*i + 1])) {
                rv[i] = Unorm8{*b}.normalized_value();
            }
            else {
                return std::nullopt;
            }
        }
        return rv;
    }
    else if (content.size() == 8) {
        // RGBA hex string (e.g. "ffaa8822")
        Color rv = Color::black();
        for (size_t i = 0; i < 4; ++i) {
            if (auto b = try_parse_hex_chars_as_byte(content[2*i], content[2*i + 1])) {
                rv[i] = Unorm8{*b}.normalized_value();
            }
            else {
                return std::nullopt;
            }
        }
        return rv;
    }
    else {
        // invalid hex string
        return std::nullopt;
    }
}

Color osc::multiply_luminance(const Color& color, float factor)
{
    ColorHSLA hsla_color{color};
    hsla_color.lightness *= factor;
    return Color{hsla_color};
}

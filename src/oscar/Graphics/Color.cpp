#include "Color.h"

#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    float calc_normalized_hsla_hue(
        float r,
        float g,
        float b,
        float,
        float max,
        float delta)
    {
        if (delta == 0.0f) {
            return 0.0f;
        }

        // figure out projection of color onto hue hexagon
        float segment = 0.0f;
        float shift = 0.0f;
        if (max == r) {
            segment = (g - b)/delta;
            shift = (segment < 0.0f ? 360.0f/60.0f : 0.0f);
        }
        else if (max == g) {
            segment = (b - r)/delta;
            shift = 120.0f/60.0f;
        }
        else {  // max == b
            segment = (r - g)/delta;
            shift = 240.0f/60.0f;
        }

        return (segment + shift)/6.0f;  // normalize
    }

    float calc_hsla_saturation(float lightness, float min, float max)
    {
        if (lightness == 0.0f) {
            return 0.0f;
        }
        else if (lightness <= 0.5f) {
            return 0.5f * (max - min)/lightness;
        }
        else if (lightness < 1.0f) {
            return 0.5f * (max - min)/(1.0f - lightness);
        }
        else {  // lightness == 1.0f
            return 0.0f;
        }
    }
}

std::ostream& osc::operator<<(std::ostream& o, const Color& c)
{
    return o << "Color(r = " << c.r << ", g = " << c.g << ", b = " << c.b << ", a = " << c.a << ')';
}

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
float osc::to_linear_colorspace(float color_channel_value)
{
    if (color_channel_value <= 0.04045f) {
        return color_channel_value / 12.92f;
    }
    else {
        return pow((color_channel_value + 0.055f) / 1.055f, 2.4f);
    }
}

float osc::to_srgb_colorspace(float color_channel_value)
{
    if (color_channel_value <= 0.0031308f) {
        return color_channel_value * 12.92f;
    }
    else {
        return pow(color_channel_value, 1.0f/2.4f)*1.055f - 0.055f;
    }
}

Color osc::to_linear_colorspace(const Color& c)
{
    return {
        to_linear_colorspace(c.r),
        to_linear_colorspace(c.g),
        to_linear_colorspace(c.b),
        c.a,
    };
}

Color osc::to_srgb_colorspace(const Color& c)
{
    return {
        to_srgb_colorspace(c.r),
        to_srgb_colorspace(c.g),
        to_srgb_colorspace(c.b),
        c.a,
    };
}

Color osc::lerp(const Color& a, const Color& b, float t)
{
    return Color{lerp(Vec4{a}, Vec4{b}, saturate(t))};
}

size_t std::hash<osc::Color>::operator()(const osc::Color& color) const
{
    return hash_of(color.r, color.g, color.b, color.a);
}

Color32 osc::to_color32(const Color& color)
{
    return to_color32(static_cast<Vec4>(color));
}

Color32 osc::to_color32(const Vec4& v)
{
    return Color32{v.x, v.y, v.z, v.w};
}

Color32 osc::to_color32(float r, float g, float b, float a)
{
    return Color32{r, g, b, a};
}

Color32 osc::to_color32(uint32_t v)
{
    return Color32{
        static_cast<uint8_t>((v >> 24) & 0xff),
        static_cast<uint8_t>((v >> 16) & 0xff),
        static_cast<uint8_t>((v >> 8) & 0xff),
        static_cast<uint8_t>((v >> 0) & 0xff),
    };
}

Color osc::to_color(Color32 c)
{
    return Color{
        c.r.normalized_value(),
        c.g.normalized_value(),
        c.b.normalized_value(),
        c.a.normalized_value(),
    };
}

Color osc::clamp_to_ldr(const Color& c)
{
    return Color{saturate(Vec4{c})};
}

ColorHSLA osc::to_hsla_color(const Color& c)
{
    // sources:
    //
    // - https://web.cs.uni-paderborn.de/cgvb/colormaster/web/color-systems/hsl.html
    // - https://stackoverflow.com/questions/39118528/rgb-to-hsl-conversion

    const auto [r, g, b, a] = clamp_to_ldr(c);
    const auto [min, max] = rgs::minmax(std::to_array({r, g, b}));  // CARE: `std::initializer_list<float>` broken in Ubuntu20?
    const float delta = max - min;

    const float hue = calc_normalized_hsla_hue(r, g, b, min, max, delta);
    const float lightness = 0.5f*(min + max);
    const float saturation = calc_hsla_saturation(lightness, min, max);

    return {hue, saturation, lightness, a};
}

Color osc::to_color(const ColorHSLA& c)
{
    // see: https://web.cs.uni-paderborn.de/cgvb/colormaster/web/color-systems/hsl.html

    const auto& [h, s, l, a] = c;

    if (l <= 0.0f) {
        return Color::black();
    }

    if (l >= 1.0f) {
        return Color::white();
    }

    const float hp = mod(6.0f*h, 6.0f);
    const float c1 = floor(hp);
    const float c2 = hp - c1;
    const float d  = l <= 0.5f ? s*l : s*(1.0f - l);
    const float u1 = l + d;
    const float u2 = l - d;
    const float u3 = u1 - (u1 - u2)*c2;
    const float u4 = u2 + (u1 - u2)*c2;

    switch (static_cast<int>(c1)) {
    default:
    case 0: return {u1, u4, u2, a};
    case 1: return {u3, u1, u2, a};
    case 2: return {u2, u1, u4, a};
    case 3: return {u2, u3, u1, a};
    case 4: return {u4, u2, u1, a};
    case 5: return {u1, u2, u3, a};
    }
}

std::string osc::to_html_string_rgba(const Color& c)
{
    std::string rv;
    rv.reserve(9);
    rv.push_back('#');
    for (auto channel : to_color32(c)) {
        auto [nibble_1, nibble_2] = to_hex_chars(static_cast<uint8_t>(channel));
        rv.push_back(nibble_1);
        rv.push_back(nibble_2);
    }
    return rv;
}

std::optional<Color> osc::try_parse_html_color_string(std::string_view v)
{
    if (v.empty()) {
        return std::nullopt;  // it's empty
    }
    if (v.front() != '#') {
        return std::nullopt;  // incorrect first character (e.g. should be "#ff0000ff")
    }

    const std::string_view content = v.substr(1);
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

Color osc::multiply_luminance(const Color& c, float factor)
{
    auto hsla = to_hsla_color(c);
    hsla.l *= factor;
    return to_color(hsla);
}

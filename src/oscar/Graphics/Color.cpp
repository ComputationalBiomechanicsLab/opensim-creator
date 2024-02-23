#include "Color.h"

#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    float CalcNormalizedHLSAHue(
        float r,
        float g,
        float b,
        float,
        float max,
        float delta)
    {
        if (delta == 0.0f)
        {
            return 0.0f;
        }

        // figure out projection of color onto hue hexagon
        float segment = 0.0f;
        float shift = 0.0f;
        if (max == r)
        {
            segment = (g - b)/delta;
            shift = (segment < 0.0f ? 360.0f/60.0f : 0.0f);
        }
        else if (max == g)
        {
            segment = (b - r)/delta;
            shift = 120.0f/60.0f;
        }
        else  // max == b
        {
            segment = (r - g)/delta;
            shift = 240.0f/60.0f;
        }

        return (segment + shift)/6.0f;  // normalize
    }

    float CalcHLSASaturation(float lightness, float min, float max)
    {
        if (lightness == 0.0f)
        {
            return 0.0f;
        }
        else if (lightness <= 0.5f)
        {
            return 0.5f * (max - min)/lightness;
        }
        else if (lightness < 1.0f)
        {
            return 0.5f * (max - min)/(1.0f - lightness);
        }
        else  // lightness == 1.0f
        {
            return 0.0f;
        }
    }
}

std::ostream& osc::operator<<(std::ostream& o, Color const& c)
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
float osc::ToLinear(float colorChannelValue)
{
    if (colorChannelValue <= 0.04045f)
    {
        return colorChannelValue / 12.92f;
    }
    else
    {
        return pow((colorChannelValue + 0.055f) / 1.055f, 2.4f);
    }
}

float osc::ToSRGB(float colorChannelValue)
{
    if (colorChannelValue <= 0.0031308f)
    {
        return colorChannelValue * 12.92f;
    }
    else
    {
        return pow(colorChannelValue, 1.0f/2.4f)*1.055f - 0.055f;
    }
}

Color osc::ToLinear(Color const& c)
{
    return
    {
        ToLinear(c.r),
        ToLinear(c.g),
        ToLinear(c.b),
        c.a,
    };
}

Color osc::ToSRGB(Color const& c)
{
    return
    {
        ToSRGB(c.r),
        ToSRGB(c.g),
        ToSRGB(c.b),
        c.a,
    };
}

Color osc::Lerp(Color const& a, Color const& b, float t)
{
    return Color{mix(Vec4{a}, Vec4{b}, saturate(t))};
}

size_t std::hash<osc::Color>::operator()(osc::Color const& color) const
{
    return HashOf(color.r, color.g, color.b, color.a);
}

Color32 osc::ToColor32(Color const& color)
{
    return ToColor32(static_cast<Vec4>(color));
}

Color32 osc::ToColor32(Vec4 const& v)
{
    return Color32{v.x, v.y, v.z, v.w};
}

Color32 osc::ToColor32(float r, float g, float b, float a)
{
    return Color32{r, g, b, a};
}

Color32 osc::ToColor32(uint32_t v)
{
    return Color32
    {
        static_cast<uint8_t>((v >> 24) & 0xff),
        static_cast<uint8_t>((v >> 16) & 0xff),
        static_cast<uint8_t>((v >> 8) & 0xff),
        static_cast<uint8_t>((v >> 0) & 0xff),
    };
}

Color osc::ToColor(Color32 c)
{
    return Color{c.r.normalized_value(), c.g.normalized_value(), c.b.normalized_value(), c.a.normalized_value()};
}

Color osc::ClampToLDR(Color const& c)
{
    return Color{saturate(Vec4{c})};
}

ColorHSLA osc::ToHSLA(Color const& c)
{
    // sources:
    //
    // - https://web.cs.uni-paderborn.de/cgvb/colormaster/web/color-systems/hsl.html
    // - https://stackoverflow.com/questions/39118528/rgb-to-hsl-conversion

    auto const [r, g, b, a] = ClampToLDR(c);
    auto const [min, max] = std::minmax({r, g, b});
    float const delta = max - min;

    float const hue = CalcNormalizedHLSAHue(r, g, b, min, max, delta);
    float const lightness = 0.5f*(min + max);
    float const saturation = CalcHLSASaturation(lightness, min, max);

    return {hue, saturation, lightness, a};
}

Color osc::ToColor(ColorHSLA const& c)
{
    // see: https://web.cs.uni-paderborn.de/cgvb/colormaster/web/color-systems/hsl.html

    auto const [h, s, l, a] = c;

    if (l <= 0.0f)
    {
        return Color::black();
    }

    if (l >= 1.0f)
    {
        return Color::white();
    }

    float const hp = mod(6.0f*h, 6.0f);
    float const c1 = floor(hp);
    float const c2 = hp - c1;
    float const d = l <= 0.5f ? s*l : s*(1.0f - l);
    float const u1 = l + d;
    float const u2 = l - d;
    float const u3 = u1 - (u1 - u2)*c2;
    float const u4 = u2 + (u1 - u2)*c2;

    switch (static_cast<int>(c1))
    {
    default:
    case 0: return {u1, u4, u2, a};
    case 1: return {u3, u1, u2, a};
    case 2: return {u2, u1, u4, a};
    case 3: return {u2, u3, u1, a};
    case 4: return {u4, u2, u1, a};
    case 5: return {u1, u2, u3, a};
    }
}

std::string osc::ToHtmlStringRGBA(Color const& c)
{
    std::string rv;
    rv.reserve(9);
    rv.push_back('#');
    for (auto v : ToColor32(c))
    {
        auto [nib1, nib2] = ToHexChars(static_cast<uint8_t>(v));
        rv.push_back(nib1);
        rv.push_back(nib2);
    }
    return rv;
}

std::optional<Color> osc::TryParseHtmlString(std::string_view v)
{
    if (v.empty())
    {
        return std::nullopt;  // it's empty
    }
    if (v.front() != '#')
    {
        return std::nullopt;  // incorrect first character (e.g. should be "#ff0000ff")
    }

    std::string_view const content = v.substr(1);
    if (content.size() == 6)
    {
        // RGB hex string
        Color rv = Color::black();
        for (size_t i = 0; i < 3; ++i)
        {
            if (auto b = TryParseHexCharsAsByte(content[2*i], content[2*i + 1]))
            {
                rv[i] = Unorm8{*b}.normalized_value();
            }
            else
            {
                return std::nullopt;
            }
        }
        return rv;
    }
    else if (content.size() == 8)
    {
        // RGBA hex string
        Color rv = Color::black();
        for (size_t i = 0; i < 4; ++i)
        {
            if (auto b = TryParseHexCharsAsByte(content[2*i], content[2*i + 1]))
            {
                rv[i] = Unorm8{*b}.normalized_value();
            }
            else
            {
                return std::nullopt;
            }
        }
        return rv;
    }
    else
    {
        // invalid hex string
        return std::nullopt;
    }
}

Color osc::MultiplyLuminance(Color const& c, float factor)
{
    auto hsla = ToHSLA(c);
    hsla.l *= factor;
    return ToColor(hsla);
}

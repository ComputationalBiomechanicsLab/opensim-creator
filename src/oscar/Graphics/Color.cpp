#include "Color.hpp"

#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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
//
// (because I am a lazy bastard)

float osc::ToLinear(float colorChannelValue) noexcept
{
    if (colorChannelValue <= 0.04045f)
    {
        return colorChannelValue / 12.92f;
    }
    else
    {
        return std::pow((colorChannelValue + 0.055f) / 1.055f, 2.4f);
    }
}

float osc::ToSRGB(float colorChannelValue) noexcept
{
    if (colorChannelValue <= 0.0031308f)
    {
        return colorChannelValue * 12.92f;
    }
    else
    {
        return std::pow(colorChannelValue, 1.0f/2.4f)*1.055f - 0.055f;
    }
}

osc::Color osc::ToLinear(Color const& c) noexcept
{
    return
    {
        ToLinear(c.r),
        ToLinear(c.g),
        ToLinear(c.b),
        c.a,
    };
}

osc::Color osc::ToSRGB(Color const& c) noexcept
{
    return
    {
        ToSRGB(c.r),
        ToSRGB(c.g),
        ToSRGB(c.b),
        c.a,
    };
}

osc::Color osc::Lerp(Color const& a, Color const& b, float t) noexcept
{
    return osc::Color{Mix(Vec4{a}, Vec4{b}, Clamp(t, 0.0f, 1.0f))};
}

size_t std::hash<osc::Color>::operator()(osc::Color const& color) const
{
    return osc::HashOf(color.r, color.g, color.b, color.a);
}

osc::Color32 osc::ToColor32(Color const& color) noexcept
{
    return ToColor32(static_cast<Vec4>(color));
}

osc::Color32 osc::ToColor32(Vec4 const& v) noexcept
{
    return Color32
    {
        ToClamped8BitColorChannel(v.r),
        ToClamped8BitColorChannel(v.g),
        ToClamped8BitColorChannel(v.b),
        ToClamped8BitColorChannel(v.a),
    };
}

osc::Color32 osc::ToColor32(float r, float g, float b, float a) noexcept
{
    return Color32
    {
        ToClamped8BitColorChannel(r),
        ToClamped8BitColorChannel(g),
        ToClamped8BitColorChannel(b),
        ToClamped8BitColorChannel(a),
    };
}

osc::Color32 osc::ToColor32(uint32_t v) noexcept
{
    return Color32
    {
        static_cast<uint8_t>((v >> 24) & 0xff),
        static_cast<uint8_t>((v >> 16) & 0xff),
        static_cast<uint8_t>((v >> 8) & 0xff),
        static_cast<uint8_t>((v >> 0) & 0xff),
    };
}

osc::Color osc::ClampToLDR(Color const& c) noexcept
{
    return Color{Clamp(Vec4{c}, 0.0f, 1.0f)};
}

std::string osc::ToHtmlStringRGBA(Color const& c)
{
    std::string rv;
    rv.reserve(9);
    rv.push_back('#');
    for (uint8_t v : ToColor32(c))
    {
        auto [nib1, nib2] = ToHexChars(v);
        rv.push_back(nib1);
        rv.push_back(nib2);
    }
    return rv;
}

std::optional<osc::Color> osc::TryParseHtmlString(std::string_view v)
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
                rv[i] = ToFloatingPointColorChannel(*b);
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
                rv[i] = ToFloatingPointColorChannel(*b);
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

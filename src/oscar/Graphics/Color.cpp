#include "Color.hpp"

#include "oscar/Utils/HashHelpers.hpp"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <cstdint>

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

float osc::ToLinear(float srgb) noexcept
{
    if (srgb <= 0.04045f)
    {
        return srgb / 12.92f;
    }
    else
    {
        return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
    }
}

float osc::ToSRGB(float linear) noexcept
{
    if (linear <= 0.0031308f)
    {
        return linear * 12.92f;
    }
    else
    {
        return std::pow(linear, 1.0f/2.4f)*1.055f - 0.055f;
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
    return osc::Color{glm::mix(glm::vec4{a}, glm::vec4{b}, glm::clamp(t, 0.0f, 1.0f))};
}

size_t std::hash<osc::Color>::operator()(osc::Color const& color) const
{
    return osc::HashOf(color.r, color.g, color.b, color.a);
}

osc::Rgba32 osc::ToRgba32(glm::vec4 const& v) noexcept
{
    return Rgba32
    {
        static_cast<uint8_t>(255.0f * v.r),
        static_cast<uint8_t>(255.0f * v.g),
        static_cast<uint8_t>(255.0f * v.b),
        static_cast<uint8_t>(255.0f * v.a),
    };
}

osc::Rgba32 osc::ToRgba32(float r, float g, float b, float a) noexcept
{
    return Rgba32
    {
        static_cast<uint8_t>(255.0f * r),
        static_cast<uint8_t>(255.0f * g),
        static_cast<uint8_t>(255.0f * b),
        static_cast<uint8_t>(255.0f * a),
    };
}

osc::Rgba32 osc::ToRgba32(std::uint32_t v) noexcept
{
    return Rgba32
    {
        static_cast<uint8_t>((v >> 24) & 0xff),
        static_cast<uint8_t>((v >> 16) & 0xff),
        static_cast<uint8_t>((v >> 8) & 0xff),
        static_cast<uint8_t>((v >> 0) & 0xff),
    };
}

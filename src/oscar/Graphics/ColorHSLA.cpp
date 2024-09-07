#include "ColorHSLA.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/CommonFunctions.h>

#include <algorithm>
#include <array>
#include <ostream>
#include <ranges>

namespace rgs = std::ranges;
using namespace osc;

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

osc::ColorHSLA::ColorHSLA(const Color& color)
{
    // sources:
    //
    // - https://web.cs.uni-paderborn.de/cgvb/colormaster/web/color-systems/hsl.html
    // - https://stackoverflow.com/questions/39118528/rgb-to-hsl-conversion

    const auto [r, g, b, a] = saturate(color);
    const auto [min, max] = rgs::minmax(std::to_array({r, g, b}));  // CARE: `std::initializer_list<float>` broken in Ubuntu20?
    const float delta = max - min;

    this->hue = calc_normalized_hsla_hue(r, g, b, min, max, delta);
    this->lightness = 0.5f*(min + max);
    this->saturation = calc_hsla_saturation(lightness, min, max);
    this->alpha = a;
}

osc::ColorHSLA::operator Color() const
{
    // see: https://web.cs.uni-paderborn.de/cgvb/colormaster/web/color-systems/hsl.html

    const auto& [h, s, l, a] = *this;

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

std::ostream& osc::operator<<(std::ostream& color, const ColorHSLA& c)
{
    return color << "ColorHSLA(h = " << c.hue << ", s = " << c.saturation << ", l = " << c.lightness << ", a = " << c.alpha << ')';
}

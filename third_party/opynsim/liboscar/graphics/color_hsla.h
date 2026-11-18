#pragma once

#include <liboscar/graphics/color.h>

#include <iosfwd>

namespace osc
{
    struct ColorHSLA final {

        ColorHSLA() = default;
        explicit ColorHSLA(const Color&);

        constexpr ColorHSLA(float hue, float saturation, float lightness, float alpha) :
            hue{hue},
            saturation{saturation},
            lightness{lightness},
            alpha{alpha}
        {}

        friend bool operator==(const ColorHSLA&, const ColorHSLA&) = default;

        explicit operator Color() const;

        float hue{};
        float saturation{};
        float lightness{};
        float alpha{};
    };

    std::ostream& operator<<(std::ostream&, const ColorHSLA&);
}

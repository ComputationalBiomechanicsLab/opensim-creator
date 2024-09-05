#pragma once

#include <oscar/Graphics/Color.h>

#include <iosfwd>

namespace osc
{
    struct ColorHSLA final {

        ColorHSLA() = default;
        explicit ColorHSLA(const Color&);

        constexpr ColorHSLA(float hue_, float saturation_, float lightness_, float alpha_) :
            hue{hue_},
            saturation{saturation_},
            lightness{lightness_},
            alpha{alpha_}
        {}

        friend bool operator==(const ColorHSLA&, const ColorHSLA&) = default;

        explicit operator Color () const;

        float hue{};
        float saturation{};
        float lightness{};
        float alpha{};
    };

    std::ostream& operator<<(std::ostream&, const ColorHSLA&);
}

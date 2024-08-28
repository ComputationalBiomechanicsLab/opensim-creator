#pragma once

#include <iosfwd>

namespace osc
{
    struct ColorHSLA final {
        friend bool operator==(const ColorHSLA&, const ColorHSLA&) = default;

        float hue = 0.0f;
        float saturation = 0.0f;
        float lightness = 0.0f;
        float alpha = 0.0f;
    };

    std::ostream& operator<<(std::ostream&, const ColorHSLA&);
}

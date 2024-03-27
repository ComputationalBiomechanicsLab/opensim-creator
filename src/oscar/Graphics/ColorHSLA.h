#pragma once

#include <iosfwd>

namespace osc
{
    struct ColorHSLA final {
        float h = 0.0f;
        float s = 0.0f;
        float l = 0.0f;
        float a = 0.0f;
    };

    std::ostream& operator<<(std::ostream&, const ColorHSLA&);
}

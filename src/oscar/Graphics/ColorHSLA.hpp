#pragma once

#include <iosfwd>

namespace osc
{
    struct ColorHSLA final {
        float h = 0.0f;
        float s = 1.0f;
        float l = 0.5f;
        float a = 1.0f;
    };

    std::ostream& operator<<(std::ostream&, ColorHSLA const&);
}

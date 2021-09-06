#pragma once

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

namespace osc {
    static constexpr float fpi = static_cast<float>(M_PI);
    static constexpr float fpi2 = fpi/2.0f;
    static constexpr float fpi4 = fpi/4.0f;
    static constexpr float f1pi = 1.0f/fpi;
}



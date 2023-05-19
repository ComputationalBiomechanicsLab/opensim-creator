#pragma once

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

namespace osc
{
    static inline float constexpr fpi = static_cast<float>(M_PI);
    static inline float constexpr fpi2 = fpi/2.0f;
    static inline float constexpr fpi4 = fpi/4.0f;
    static inline float constexpr f1pi = 1.0f/fpi;
}

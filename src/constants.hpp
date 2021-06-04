#pragma once

namespace osc {
    // This is defined here, rather than just using M_PI, because different platforms
    // seem to define M_PI it in different headers, and because it's defined as a `double`
    // literal, so needs `static_casting` everywhere when using strict compiler flags.
    //
    // I'm *assuming* the first ~20 or so significant digits of PI will not change
    // much ;)
    static constexpr float pi_f = static_cast<float>(3.14159265358979323846);
}

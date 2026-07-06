#pragma once

#include <liboscar/utilities/c_string_view.h>

namespace opyn
{
    /// Represents the integrator methodologies supported by the backend.
    enum class IntegratorMethod {
        RungeKuttaMerson,
        ExplicitEuler,
        RungeKutta2,
        RungeKutta3,
        RungeKuttaFeldberg,
        SemiExplicitEuler2,
        Verlet,
        NUM_OPTIONS,

        Default = RungeKuttaMerson,
    };

    /// Returns a human-readable label (no decorations) for `integrator_method`.
    osc::CStringView label_for(const IntegratorMethod& integrator_method);
}

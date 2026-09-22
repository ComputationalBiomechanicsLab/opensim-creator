#include "integrator_method.h"

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>

osc::CStringView opyn::label_for(const IntegratorMethod& integrator_method)
{
    static_assert(osc::num_options<IntegratorMethod>() == 7);
    switch (integrator_method) {
    case IntegratorMethod::RungeKuttaMerson:   return "Runge Kutta Merson";
    case IntegratorMethod::ExplicitEuler:      return "Explicit Euler";
    case IntegratorMethod::RungeKutta2:        return "Runge Kutta 2";
    case IntegratorMethod::RungeKutta3:        return "Runge Kutta 3";
    case IntegratorMethod::RungeKuttaFeldberg: return "Runge Kutta Feldberg";
    case IntegratorMethod::SemiExplicitEuler2: return "Semi Explicit Euler 2";
    case IntegratorMethod::Verlet:             return "Verlet";
    default:                                   return "Unknown";
    }
}

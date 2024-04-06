#include "IntegratorMethod.h"

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <simmath/ExplicitEulerIntegrator.h>
#include <simmath/RungeKutta2Integrator.h>
#include <simmath/RungeKutta3Integrator.h>
#include <simmath/RungeKuttaFeldbergIntegrator.h>
#include <simmath/RungeKuttaMersonIntegrator.h>
#include <simmath/SemiExplicitEuler2Integrator.h>
#include <simmath/VerletIntegrator.h>

#include <array>
#include <memory>

using namespace osc;
using osc::detail::IntegratorMethodOption;

namespace
{
    constexpr auto c_IntegratorMethodOptionStrings = std::to_array<CStringView>({
        "OpenSim::Manager Default",
        "Explicit Euler",
        "Runge Kutta 2",
        "Runge Kutta 3",
        "Runge Kutta Feldberg",
        "Runge Kutta Merson",
        "Semi Explicit Euler 2",
        "Verlet",
    });

    template<std::derived_from<SimTK::Integrator> Integrator>
    std::unique_ptr<SimTK::Integrator> make_integrator(SimTK::System const& system)
    {
        return std::make_unique<Integrator>(system);
    }

    using IntegratorCtor = std::unique_ptr<SimTK::Integrator>(*)(SimTK::System const&);

    constexpr auto c_IntegratorMethodConstructors = std::to_array<IntegratorCtor>({
        make_integrator<SimTK::RungeKuttaMersonIntegrator>,
        make_integrator<SimTK::ExplicitEulerIntegrator>,
        make_integrator<SimTK::RungeKutta2Integrator>,
        make_integrator<SimTK::RungeKutta3Integrator>,
        make_integrator<SimTK::RungeKuttaFeldbergIntegrator>,
        make_integrator<SimTK::RungeKuttaMersonIntegrator>,
        make_integrator<SimTK::SemiExplicitEuler2Integrator>,
        make_integrator<SimTK::VerletIntegrator>,
    });
}

CStringView osc::IntegratorMethod::label() const
{
    static_assert(c_IntegratorMethodOptionStrings.size() == num_options<IntegratorMethodOption>());
    return c_IntegratorMethodOptionStrings[to_index(m_Option)];
}

std::unique_ptr<SimTK::Integrator> osc::IntegratorMethod::instantiate(SimTK::System const& system) const
{
    static_assert(c_IntegratorMethodConstructors.size() == num_options<IntegratorMethodOption>());
    return c_IntegratorMethodConstructors[to_index(m_Option)](system);
}

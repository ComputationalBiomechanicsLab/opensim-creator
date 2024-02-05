#include "IntegratorMethod.hpp"

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <simmath/ExplicitEulerIntegrator.h>
#include <simmath/RungeKutta2Integrator.h>
#include <simmath/RungeKutta3Integrator.h>
#include <simmath/RungeKuttaFeldbergIntegrator.h>
#include <simmath/RungeKuttaMersonIntegrator.h>
#include <simmath/SemiExplicitEuler2Integrator.h>
#include <simmath/VerletIntegrator.h>

#include <array>
#include <cstddef>
#include <memory>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_IntegratorMethods = std::to_array<IntegratorMethod>(
    {
        IntegratorMethod::OpenSimManagerDefault,
        IntegratorMethod::ExplicitEuler,
        IntegratorMethod::RungeKutta2,
        IntegratorMethod::RungeKutta3,
        IntegratorMethod::RungeKuttaFeldberg,
        IntegratorMethod::RungeKuttaMerson,
        IntegratorMethod::SemiExplicitEuler2,
        IntegratorMethod::Verlet,
    });
    static_assert(c_IntegratorMethods.size() == NumOptions<IntegratorMethod>());

    constexpr auto c_IntegratorMethodStrings = std::to_array<CStringView>(
    {
        "OpenSim::Manager Default",
        "Explicit Euler",
        "Runge Kutta 2",
        "Runge Kutta 3",
        "Runge Kutta Feldberg",
        "Runge Kutta Merson",
        "Semi Explicit Euler 2",
        "Verlet",
    });
    static_assert(c_IntegratorMethodStrings.size() == NumOptions<IntegratorMethod>());
}

std::span<IntegratorMethod const> osc::GetAllIntegratorMethods()
{
    return c_IntegratorMethods;
}

std::span<CStringView const> osc::GetAllIntegratorMethodStrings()
{
    return c_IntegratorMethodStrings;
}

CStringView osc::GetIntegratorMethodString(IntegratorMethod im)
{
    return GetAllIntegratorMethodStrings()[static_cast<size_t>(im)];
}

std::unique_ptr<SimTK::Integrator> osc::CreateIntegrator(SimTK::System const& system, IntegratorMethod method)
{
    switch (method) {
    case IntegratorMethod::OpenSimManagerDefault:
        return std::make_unique<SimTK::RungeKuttaMersonIntegrator>(system);
    case IntegratorMethod::ExplicitEuler:
        return std::make_unique<SimTK::ExplicitEulerIntegrator>(system);
    case IntegratorMethod::RungeKutta2:
        return std::make_unique<SimTK::RungeKutta2Integrator>(system);
    case IntegratorMethod::RungeKutta3:
        return std::make_unique<SimTK::RungeKutta3Integrator>(system);
    case IntegratorMethod::RungeKuttaFeldberg:
        return std::make_unique<SimTK::RungeKuttaFeldbergIntegrator>(system);
    case IntegratorMethod::RungeKuttaMerson:
        return std::make_unique<SimTK::RungeKuttaMersonIntegrator>(system);
    case IntegratorMethod::SemiExplicitEuler2:
        return std::make_unique<SimTK::SemiExplicitEuler2Integrator>(system);
    case IntegratorMethod::Verlet:
        return std::make_unique<SimTK::VerletIntegrator>(system);
    default:
        return std::make_unique<SimTK::RungeKuttaMersonIntegrator>(system);
    }
}

#include "IntegratorMethod.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <nonstd/span.hpp>
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

namespace
{
    auto constexpr c_IntegratorMethods = osc::to_array<osc::IntegratorMethod>(
    {
        osc::IntegratorMethod::OpenSimManagerDefault,
        osc::IntegratorMethod::ExplicitEuler,
        osc::IntegratorMethod::RungeKutta2,
        osc::IntegratorMethod::RungeKutta3,
        osc::IntegratorMethod::RungeKuttaFeldberg,
        osc::IntegratorMethod::RungeKuttaMerson,
        osc::IntegratorMethod::SemiExplicitEuler2,
        osc::IntegratorMethod::Verlet,
    });
    static_assert(c_IntegratorMethods.size() == static_cast<size_t>(osc::IntegratorMethod::TOTAL));

    auto constexpr c_IntegratorMethodStrings = osc::to_array<osc::CStringView>(
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
    static_assert(c_IntegratorMethodStrings.size() == static_cast<size_t>(osc::IntegratorMethod::TOTAL));
}

nonstd::span<osc::IntegratorMethod const> osc::GetAllIntegratorMethods()
{
    return c_IntegratorMethods;
}

nonstd::span<osc::CStringView const> osc::GetAllIntegratorMethodStrings()
{
    return c_IntegratorMethodStrings;
}

osc::CStringView osc::GetIntegratorMethodString(IntegratorMethod im)
{
    return GetAllIntegratorMethodStrings()[static_cast<size_t>(im)];
}

std::unique_ptr<SimTK::Integrator> osc::CreateIntegrator(SimTK::System const& system, osc::IntegratorMethod method)
{
    switch (method) {
    case osc::IntegratorMethod::OpenSimManagerDefault:
        return std::make_unique<SimTK::RungeKuttaMersonIntegrator>(system);
    case osc::IntegratorMethod::ExplicitEuler:
        return std::make_unique<SimTK::ExplicitEulerIntegrator>(system);
    case osc::IntegratorMethod::RungeKutta2:
        return std::make_unique<SimTK::RungeKutta2Integrator>(system);
    case osc::IntegratorMethod::RungeKutta3:
        return std::make_unique<SimTK::RungeKutta3Integrator>(system);
    case osc::IntegratorMethod::RungeKuttaFeldberg:
        return std::make_unique<SimTK::RungeKuttaFeldbergIntegrator>(system);
    case osc::IntegratorMethod::RungeKuttaMerson:
        return std::make_unique<SimTK::RungeKuttaMersonIntegrator>(system);
    case osc::IntegratorMethod::SemiExplicitEuler2:
        return std::make_unique<SimTK::SemiExplicitEuler2Integrator>(system);
    case osc::IntegratorMethod::Verlet:
        return std::make_unique<SimTK::VerletIntegrator>(system);
    default:
        return std::make_unique<SimTK::RungeKuttaMersonIntegrator>(system);
    }
}

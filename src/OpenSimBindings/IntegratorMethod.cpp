#include "IntegratorMethod.hpp"

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

static std::array<osc::IntegratorMethod, static_cast<size_t>(osc::IntegratorMethod::TOTAL)> CreateIntegratorMethodsLut()
{
    return {
        osc::IntegratorMethod::OpenSimManagerDefault,
        osc::IntegratorMethod::ExplicitEuler,
        osc::IntegratorMethod::RungeKutta2,
        osc::IntegratorMethod::RungeKutta3,
        osc::IntegratorMethod::RungeKuttaFeldberg,
        osc::IntegratorMethod::RungeKuttaMerson,
        osc::IntegratorMethod::SemiExplicitEuler2,
        osc::IntegratorMethod::Verlet,
    };
}

static std::array<char const*, static_cast<size_t>(osc::IntegratorMethod::TOTAL)> CreateIntegratorMethodStringsLut()
{
    return {
        "OpenSim::Manager Default",
        "Explicit Euler",
        "Runge Kutta 2",
        "Runge Kutta 3",
        "Runge Kutta Feldberg",
        "Runge Kutta Merson",
        "Semi Explicit Euler 2",
        "Verlet",
    };
}

nonstd::span<osc::IntegratorMethod const> osc::GetAllIntegratorMethods()
{
    static auto const g_IntegratorMethods = CreateIntegratorMethodsLut();
    return g_IntegratorMethods;
}

nonstd::span<char const* const> osc::GetAllIntegratorMethodStrings()
{
    static auto const g_IntegratorMethodStrings = CreateIntegratorMethodStringsLut();
    return g_IntegratorMethodStrings;
}

char const* osc::GetIntegratorMethodString(IntegratorMethod im)
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

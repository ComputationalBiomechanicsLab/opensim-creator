#pragma once

#include <nonstd/span.hpp>

#include <memory>

namespace SimTK
{
    class Integrator;
    class System;
}

namespace osc
{
    // integration methods that are supported by the OpenSim backend
    enum class IntegratorMethod
    {
        OpenSimManagerDefault = 0,
        ExplicitEuler,
        RungeKutta2,
        RungeKutta3,
        RungeKuttaFeldberg,
        RungeKuttaMerson,
        SemiExplicitEuler2,
        Verlet,
        TOTAL,
    };

    nonstd::span<IntegratorMethod const> GetAllIntegratorMethods();
    nonstd::span<char const* const> GetAllIntegratorMethodStrings();
    char const* GetIntegratorMethodString(IntegratorMethod);
    std::unique_ptr<SimTK::Integrator> CreateIntegrator(SimTK::System const&, IntegratorMethod);
}


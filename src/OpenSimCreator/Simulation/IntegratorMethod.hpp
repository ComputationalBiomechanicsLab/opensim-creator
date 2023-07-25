#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <nonstd/span.hpp>

#include <memory>

namespace SimTK { class Integrator; }
namespace SimTK { class System; }

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
        NUM_OPTIONS,
    };

    nonstd::span<IntegratorMethod const> GetAllIntegratorMethods();
    nonstd::span<CStringView const> GetAllIntegratorMethodStrings();
    CStringView GetIntegratorMethodString(IntegratorMethod);
    std::unique_ptr<SimTK::Integrator> CreateIntegrator(SimTK::System const&, IntegratorMethod);
}


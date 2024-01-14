#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <memory>
#include <span>

namespace SimTK { class Integrator; }
namespace SimTK { class System; }

namespace osc
{
    // integration methods that are supported by the OpenSim backend
    enum class IntegratorMethod {
        OpenSimManagerDefault,
        ExplicitEuler,
        RungeKutta2,
        RungeKutta3,
        RungeKuttaFeldberg,
        RungeKuttaMerson,
        SemiExplicitEuler2,
        Verlet,
        NUM_OPTIONS,
    };

    std::span<IntegratorMethod const> GetAllIntegratorMethods();
    std::span<CStringView const> GetAllIntegratorMethodStrings();
    CStringView GetIntegratorMethodString(IntegratorMethod);
    std::unique_ptr<SimTK::Integrator> CreateIntegrator(SimTK::System const&, IntegratorMethod);
}


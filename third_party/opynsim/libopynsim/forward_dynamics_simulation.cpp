#include "forward_dynamics_simulation.h"

#include <libopynsim/integrator_settings.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/enum_helpers.h>
#include <simmath/Integrator.h>
#include <simmath/TimeStepper.h>

#include <memory>
#include <stdexcept>
#include <utility>

using namespace opyn;

namespace
{
    std::unique_ptr<SimTK::Integrator> make_simtk_integrator_with_method(
        const IntegratorMethod& method,
        const SimTK::MultibodySystem& system)
    {
        static_assert(osc::num_options<IntegratorMethod>() == 7);
        switch (method) {
        case IntegratorMethod::RungeKuttaMerson:   return std::make_unique<SimTK::RungeKuttaMersonIntegrator>(system);
        case IntegratorMethod::ExplicitEuler:      return std::make_unique<SimTK::ExplicitEulerIntegrator>(system);
        case IntegratorMethod::RungeKutta2:        return std::make_unique<SimTK::RungeKutta2Integrator>(system);
        case IntegratorMethod::RungeKutta3:        return std::make_unique<SimTK::RungeKutta3Integrator>(system);
        case IntegratorMethod::RungeKuttaFeldberg: return std::make_unique<SimTK::RungeKuttaFeldbergIntegrator>(system);
        case IntegratorMethod::SemiExplicitEuler2: return std::make_unique<SimTK::SemiExplicitEuler2Integrator>(system);
        case IntegratorMethod::Verlet:             return std::make_unique<SimTK::VerletIntegrator>(system);
        default:                                   std::unreachable();
        }
    }

    std::unique_ptr<SimTK::Integrator> make_simtk_integrator(
        const IntegratorSettings& settings,
        const SimTK::MultibodySystem& system)
    {
        auto rv = make_simtk_integrator_with_method(settings.method, system);
        rv->setInternalStepLimit(static_cast<int>(settings.internal_step_limit));
        rv->setMinimumStepSize(settings.minimum_step_size);
        rv->setMaximumStepSize(settings.maximum_step_size);
        rv->setAccuracy(settings.accuracy);
        // rv->setFinalTime(end_time);
        // rv->setReturnEveryInternalStep(true);
        return rv;
    }

    SimTK::TimeStepper create_simtk_time_stepper(
        const SimTK::MultibodySystem& system,
        SimTK::Integrator& integrator,
        const SimTK::State& state)
    {
        SimTK::TimeStepper rv{system, integrator};
        // rv.setReportAllSignificantStates(true);
        rv.initialize(state);
        return rv;
    }
}

class opyn::ForwardDynamicsSimulation::Impl final {
public:
    Impl(
        Model model,
        ModelState model_state,
        const std::optional<IntegratorSettings>& integrator_settings) :
        model_{std::move(model)},
        state_{std::move(model_state)},
        integrator_{make_simtk_integrator(integrator_settings.value_or(IntegratorSettings{}), model_.open_sim_model().getMultibodySystem())},
        time_stepper_{create_simtk_time_stepper(model_.open_sim_model().getMultibodySystem(), *integrator_, state_.simbody_state())}
    {}

    ModelState integrate_to(double time, ModelStateStage realized_to)
    {
        OSC_ASSERT_ALWAYS(time >= integrator_->getTime() && "The provided time must be greater or equal to the simulation's current time.");

        const auto step_status = time_stepper_.stepTo(time);

        // The final time is infinity. Therefore, only reason a simulation
        // can end is if there's a problem.
        if (integrator_->isSimulationOver()) {
            throw std::runtime_error{integrator_->getTerminationReasonString(integrator_->getTerminationReason())};
        }
        if (step_status != SimTK::Integrator::ReachedReportTime) {
            throw std::runtime_error{"The integrator was unable to `integrate_to` the specified time (t)."};
        }
        state_.upd_simbody_state() = integrator_->getState();
        model_.realize(state_, realized_to);
        return state_;  // Return a copy
    }

    Model model_;
    ModelState state_;
    std::unique_ptr<SimTK::Integrator> integrator_;
    SimTK::TimeStepper time_stepper_;
};

opyn::ForwardDynamicsSimulation::ForwardDynamicsSimulation(
    Model model,
    ModelState model_state,
    const std::optional<IntegratorSettings>& integrator_settings) :
    impl_{std::make_unique<Impl>(std::move(model), std::move(model_state), integrator_settings)}
{}
opyn::ForwardDynamicsSimulation::ForwardDynamicsSimulation(ForwardDynamicsSimulation&&) noexcept = default;
opyn::ForwardDynamicsSimulation::~ForwardDynamicsSimulation() noexcept = default;
opyn::ForwardDynamicsSimulation& opyn::ForwardDynamicsSimulation::operator=(ForwardDynamicsSimulation&&) noexcept = default;

ModelState opyn::ForwardDynamicsSimulation::integrate_to(double time, ModelStateStage realized_to) { return impl_->integrate_to(time, realized_to); }

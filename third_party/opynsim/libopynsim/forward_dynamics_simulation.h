#pragma once

#include <libopynsim/integrator_settings.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model.h>

#include <memory>
#include <optional>

namespace opyn
{
    /// Represents an active forward-dynamics simulation.
    ///
    /// The simulation stores a `ModelState` that it integrates forward
    /// in time to a caller-specified timepoint (see `integrate_to`).
    class ForwardDynamicsSimulation final {
    public:
        /// Constructs a `ForwardDynamicsSimulation` of `model` that starts at
        /// `model_state` and internally uses an integrator configured with
        /// `integrator_settings` (if provided).
        explicit ForwardDynamicsSimulation(
            Model model,
            ModelState model_state,
            const std::optional<IntegratorSettings>& = std::nullopt
        );
        ForwardDynamicsSimulation(const ForwardDynamicsSimulation&) = delete;
        ForwardDynamicsSimulation(ForwardDynamicsSimulation&&) noexcept;
        ~ForwardDynamicsSimulation() noexcept;

        ForwardDynamicsSimulation& operator=(const ForwardDynamicsSimulation&) = delete;
        ForwardDynamicsSimulation& operator=(ForwardDynamicsSimulation&&) noexcept;

        /// Integrates this simulation's internal `ModelState` to `time` and returns
        /// a copy of it realized to at least `realized_to`.
        ModelState integrate_to(double time, ModelStateStage realized_to = ModelStateStage::report);

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

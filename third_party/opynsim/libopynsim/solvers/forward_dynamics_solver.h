#pragma once

#include <libopynsim/solvers/integrator_settings.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model.h>

#include <memory>
#include <optional>

namespace opyn
{
    /// Integrates the forward dynamics of a model + state pair.
    ///
    /// The solver manages a `ModelState` that it integrates forward
    /// in time to a caller-specified timepoint (see `integrate_to`).
    class ForwardDynamicsSolver final {
    public:
        /// Constructs a `ForwardDynamicsSolver` for the given `model`. If
        /// supplied, the  solver manages `model_state`; otherwise, it
        /// manages `model.initial_state()`. The solver an integrator
        /// configured with `integrator_settings` (if provided); otherwise, it
        /// uses a default integrator configuration.
        explicit ForwardDynamicsSolver(
            Model model,
            std::optional<ModelState> model_state = std::nullopt,
            const std::optional<IntegratorSettings>& = std::nullopt);
        ForwardDynamicsSolver(const ForwardDynamicsSolver&) = delete;
        ForwardDynamicsSolver(ForwardDynamicsSolver&&) noexcept;
        ~ForwardDynamicsSolver() noexcept;
        ForwardDynamicsSolver& operator=(const ForwardDynamicsSolver&) = delete;
        ForwardDynamicsSolver& operator=(ForwardDynamicsSolver&&) noexcept;

        /// Returns a reference to the internal `ModelState` managed by this solver.
        const ModelState& model_state() const;

        /// Solves the forward dynamics of the managed `ModelState` to `time`
        /// and returns a copy of it realized to at least `realized_to`.
        ModelState integrate_to(double time, ModelStateStage realized_to = ModelStateStage::report);

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

#pragma once

#include <libopynsim/integrator_settings.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model.h>

#include <memory>
#include <optional>

namespace opyn
{
    /// Integrates the forward dynamics of a model + state pair.
    ///
    /// The solver stores a `ModelState` that it integrates forward
    /// in time to a caller-specified timepoint (see `integrate_to`).
    class ForwardDynamicsSolver final {
    public:

        /// Constructs a `ForwardDynamicsSolver` that starts with the
        /// given `model` and `model_state` pair and uses an integrator
        /// configured with `integrator_settings` (if provided).
        explicit ForwardDynamicsSolver(
            Model model,
            ModelState model_state,
            const std::optional<IntegratorSettings>& = std::nullopt
        );
        ForwardDynamicsSolver(const ForwardDynamicsSolver&) = delete;
        ForwardDynamicsSolver(ForwardDynamicsSolver&&) noexcept;
        ~ForwardDynamicsSolver() noexcept;

        ForwardDynamicsSolver& operator=(const ForwardDynamicsSolver&) = delete;
        ForwardDynamicsSolver& operator=(ForwardDynamicsSolver&&) noexcept;

        /// Solves the forward dynamics of the internal `ModelState` to `time`
        /// and returns a copy of it realized to at least `realized_to`.
        ModelState integrate_to(double time, ModelStateStage realized_to = ModelStateStage::report);

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

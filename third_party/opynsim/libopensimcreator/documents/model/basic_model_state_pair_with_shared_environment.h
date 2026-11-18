#pragma once

#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>

#include <liboscar/utilities/clone_ptr.h>

#include <filesystem>
#include <memory>

namespace OpenSim
{
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    class Environment;

    // an `OpenSim::Model` + `SimTK::State` that is a value type, constructed with:
    //
    // - `osc::Initialize`
    // - (if creating a new state)  `try_equilibrate_muscles_or_log_warning(model, state)`
    // - (if creating a new state) `model.realizeDynamics(State&)` / `model.realizeReport(State&)`
    //
    // this is a *basic* class that only guarantees the model is *initialized* this way. It
    // does not guarantee that everything is up-to-date after a caller mutates the model.
    class BasicModelStatePairWithSharedEnvironment final
        : public ModelStatePairWithSharedEnvironment {
    public:
        // clang-format off
        BasicModelStatePairWithSharedEnvironment();
        explicit BasicModelStatePairWithSharedEnvironment(const ModelStatePairWithSharedEnvironment&);
        explicit BasicModelStatePairWithSharedEnvironment(const std::filesystem::path&);
        explicit BasicModelStatePairWithSharedEnvironment(OpenSim::Model&&);
        BasicModelStatePairWithSharedEnvironment(const OpenSim::Model&, const SimTK::State&);
        BasicModelStatePairWithSharedEnvironment(const BasicModelStatePairWithSharedEnvironment&);
        BasicModelStatePairWithSharedEnvironment(BasicModelStatePairWithSharedEnvironment&&) noexcept;
        BasicModelStatePairWithSharedEnvironment& operator=(const BasicModelStatePairWithSharedEnvironment&);
        BasicModelStatePairWithSharedEnvironment& operator=(BasicModelStatePairWithSharedEnvironment&&) noexcept;
        ~BasicModelStatePairWithSharedEnvironment() noexcept override;
        // clang-format on

    private:
        const OpenSim::Model& impl_get_model() const override;
        const SimTK::State& impl_get_state() const override;

        float impl_get_fixup_scale_factor() const override;
        void impl_set_fixup_scale_factor(float) override;

        std::shared_ptr<Environment> impl_upd_associated_environment() const override;

        class Impl;
        ClonePtr<Impl> impl_;
    };
}

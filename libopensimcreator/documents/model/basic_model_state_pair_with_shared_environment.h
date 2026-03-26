#pragma once

#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>

#include <liboscar/utilities/clone_ptr.h>

#include <filesystem>
#include <memory>

namespace OpenSim { class Model; }
namespace osc { class Environment; }
namespace SimTK { class State; }

namespace osc
{
    // an `OpenSim::Model` + `SimTK::State` that is a value type, constructed with:
    //
    // - `osc::Initialize`
    // - (if creating a new state) `TryEquilibrateMusclesOrLogWarning(model, state)`
    // - (if creating a new state) `model.realizeDynamics(State&)` / `model.realizeReport(State&)`
    //
    // this is a *basic* class that only guarantees the model is *initialized* this way. It
    // does not guarantee that everything is up-to-date after a caller mutates the model.
    class BasicModelStatePairWithSharedEnvironment final : public ModelStatePairWithSharedEnvironment {
    public:
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

    private:
        const OpenSim::Model& implGetModel() const final;
        const SimTK::State& implGetState() const final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        std::shared_ptr<Environment> implUpdAssociatedEnvironment() const final;

        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}

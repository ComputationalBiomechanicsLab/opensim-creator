#pragma once

#include <libopynsim/documents/model/model_state_pair.h>

#include <liboscar/utilities/clone_ptr.h>

#include <filesystem>

namespace OpenSim { class Model; }
namespace SimTK { class State; }

namespace opyn
{
    // an `OpenSim::Model` + `SimTK::State` that is a value type, constructed with:
    //
    // - `opyn::Initialize`
    // - (if creating a new state) `TryEquilibrateMusclesOrLogWarning(model, state)`
    // - (if creating a new state) `model.realizeDynamics(State&)` / `model.realizeReport(State&)`
    //
    // this is a *basic* class that only guarantees the model is *initialized* this way. It
    // does not guarantee that everything is up-to-date after a caller mutates the model.
    class BasicModelStatePair : public ModelStatePair {
    public:
        BasicModelStatePair();
        explicit BasicModelStatePair(const ModelStatePair&);
        explicit BasicModelStatePair(const std::filesystem::path&);
        explicit BasicModelStatePair(OpenSim::Model&&);
        BasicModelStatePair(const OpenSim::Model&, const SimTK::State&);
        BasicModelStatePair(const BasicModelStatePair&);
        BasicModelStatePair(BasicModelStatePair&&) noexcept;
        BasicModelStatePair& operator=(const BasicModelStatePair&);
        BasicModelStatePair& operator=(BasicModelStatePair&&) noexcept;
        ~BasicModelStatePair() noexcept override;

    private:
        const OpenSim::Model& implGetModel() const final;
        const SimTK::State& implGetState() const final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        class Impl;
        osc::ClonePtr<Impl> m_Impl;
    };
}

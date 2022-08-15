#pragma once

#include "src/OpenSimBindings/VirtualModelStatePair.hpp"
#include "src/Utils/ClonePtr.hpp"

namespace OpenSim { class Model; }
namespace SimTK { class State; }

namespace osc
{
    // an `OpenSim::Model` + `SimTK::State` that is a value type, constructed with:
    //
    // - `osc::Initialize`
    // - (if creating a new state) `model.equilibrateMuscles(State&)`
    // - (if creating a new state) `model.realizeAcceleration(State&)`
    //
    // this is a *basic* class that only guarantees the model is *initialized* this way. It
    // does not guarantee that everything is up-to-date after a caller mutates the model.
    class BasicModelStatePair final : public VirtualModelStatePair {
    public:
        explicit BasicModelStatePair(VirtualModelStatePair const&);
        BasicModelStatePair(OpenSim::Model const&, SimTK::State const&);
        BasicModelStatePair(BasicModelStatePair const&);
        BasicModelStatePair(BasicModelStatePair&&) noexcept;
        BasicModelStatePair& operator=(BasicModelStatePair const&);
        BasicModelStatePair& operator=(BasicModelStatePair&&) noexcept;
        ~BasicModelStatePair() noexcept override;

        OpenSim::Model const& getModel() const override;
        SimTK::State const& getState() const override;

        float getFixupScaleFactor() const override;
        void setFixupScaleFactor(float) override;

    private:
        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}

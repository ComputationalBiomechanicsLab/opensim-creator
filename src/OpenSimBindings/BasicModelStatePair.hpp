#pragma once

#include "src/OpenSimBindings/VirtualModelStatePair.hpp"
#include "src/Utils/ClonePtr.hpp"

#include <memory>
#include <string_view>

namespace SimTK
{
    class State;
}

namespace OpenSim
{
    class Model;
}

namespace osc
{
    // an `OpenSim::Model` + `SimTK::State` that is a value type, constructed with:
    //
    // - `model.finalizeFromProperties`
    // - `model.finalizeConnections`
    // - `model.buildSystem`
    // - (if creating a new state) `model.equilibrateMuscles(State&)`
    // - (if creating a new state) `model.realizeAcceleration(State&)`
    //
    // this is a *basic* class that only guarantees the model is *initialized* this way. It
    // does not guarantee that everything is up-to-date after a caller mutates the model.
    class BasicModelStatePair final : VirtualModelStatePair {
    public:
        BasicModelStatePair();
        BasicModelStatePair(std::string_view osimPath);
        BasicModelStatePair(std::unique_ptr<OpenSim::Model>);
        BasicModelStatePair(OpenSim::Model const&, SimTK::State const&);  // copies
        BasicModelStatePair(BasicModelStatePair const&);
        BasicModelStatePair(BasicModelStatePair&&) noexcept;
        BasicModelStatePair& operator=(BasicModelStatePair const&);
        BasicModelStatePair& operator=(BasicModelStatePair&&) noexcept;
        ~BasicModelStatePair() noexcept override;

        OpenSim::Model const& getModel() const override;
        OpenSim::Model& updModel() override;

        SimTK::State const& getState() const override;

        class Impl;
    private:
        ClonePtr<Impl> m_Impl;
    };
}

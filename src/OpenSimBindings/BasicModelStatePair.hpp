#pragma once

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
    // guarantees to be a value type that is constructed with:
    //
    // - model called `finalizeFromProperties`
    // - model called `finalizeConnections`
    // - model called `buildSystem`
    // - (if creating a state) state called `Model::equilibrateMuscles(State&)`
    // - (if creating a state) state called `Model::realizeAcceleration(State&)`
    //
    // does not maintain these promises throughout its lifetime (callers can
    // freely mutate both the model and state)
    class BasicModelStatePair final {
    public:
        BasicModelStatePair();
        BasicModelStatePair(std::string_view osimPath);
        BasicModelStatePair(std::unique_ptr<OpenSim::Model>);
        BasicModelStatePair(OpenSim::Model const&, SimTK::State const&);  // copies
        BasicModelStatePair(BasicModelStatePair const&);
        BasicModelStatePair(BasicModelStatePair&&) noexcept;
        BasicModelStatePair& operator=(BasicModelStatePair const&);
        BasicModelStatePair& operator=(BasicModelStatePair&&) noexcept;
        ~BasicModelStatePair() noexcept;

        OpenSim::Model const& getModel() const;
        OpenSim::Model& updModel();
        SimTK::State const& getState() const;
        SimTK::State& updState();

        class Impl;
    private:
        ClonePtr<Impl> m_Impl;
    };
}

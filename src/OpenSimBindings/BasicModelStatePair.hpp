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

/* TODO
 *
 * #include "src/Utils/UID.hpp"

    // guarantees that the type maintains:
    //
    // - model called `finalizeFromProperties`
    // - model called `finalizeConnections`
    // - model called `buildSystem`
    // - state called `Model::equilibrateMuscles(State&)`
    // - state called `Model::realizeAcceleration(State&)`
    //
    // throughout its lifetime by dirty-flagging any potentially mutable accesses
    class ModelStatePair final {
    public:
        ModelStatePair();
        explicit ModelStatePair(std::string const& osimPath);
        explicit ModelStatePair(std::unique_ptr<OpenSim::Model>);
        ModelStatePair(ModelStatePair const&);
        ModelStatePair(ModelStatePair&&) noexcept;
        ModelStatePair& operator=(ModelStatePair const&);
        ModelStatePair& operator=(ModelStatePair&&) noexcept;
        ~ModelStatePair() noexcept;

        UID getID() const;
        UID getVersion() const;

        bool isDirty() const;  // indicates that the model may update next time a getter/setter is called
        void update();  // manually performs an update of the model

        OpenSim::Model const& getModel() const;
        OpenSim::Model& updModel();
        OpenSim::Model& peekModelADVANCED();
        void markModelAsModifiedADVANCED();
        void setModel(std::unique_ptr<OpenSim::Model>);
        UID getModelVersion() const;

        SimTK::State const& getState() const;
        SimTK::State& updState();
        SimTK::State& peekStateADVANCED();
        void markStateAsModifiedADVANCED();
        UID getStateVersion() const;

        class Impl;
    private:
        friend bool operator==(ModelStatePair const&, ModelStatePair const&);
        friend bool operator!=(ModelStatePair const&, ModelStatePair const&);
        friend bool operator<(ModelStatePair const&, ModelStatePair const&);
        friend bool operator<=(ModelStatePair const&, ModelStatePair const&);
        friend bool operator>(ModelStatePair const&, ModelStatePair const&);
        friend bool operator>=(ModelStatePair const&, ModelStatePair const&);
        friend struct std::hash<ModelStatePair>;

        ClonePtr<Impl> m_Impl;
    };

    bool operator==(ModelStatePair const&, ModelStatePair const&);
    bool operator!=(ModelStatePair const&, ModelStatePair const&);
    bool operator<(ModelStatePair const&, ModelStatePair const&);
    bool operator<=(ModelStatePair const&, ModelStatePair const&);
    bool operator>(ModelStatePair const&, ModelStatePair const&);
    bool operator>=(ModelStatePair const&, ModelStatePair const&);
}

namespace std
{
    template<>
    struct hash<osc::ModelStatePair> {
        std::size_t operator()(osc::ModelStatePair const&) const;
    };
}
*/

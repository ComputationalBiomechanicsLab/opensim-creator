#pragma once

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>

#include <liboscar/Utils/UID.h>

#include <memory>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace osc { class Environment; }
namespace osc { class Simulation; }
namespace SimTK { class State; }

namespace osc
{
    // a readonly model+state pair from a particular step from a simulator
    class SimulationModelStatePair final : public IModelStatePair {
    public:
        SimulationModelStatePair();
        SimulationModelStatePair(std::shared_ptr<Simulation>, SimulationReport);
        SimulationModelStatePair(const SimulationModelStatePair&) = delete;
        SimulationModelStatePair(SimulationModelStatePair&&) noexcept;
        SimulationModelStatePair& operator=(const SimulationModelStatePair&) = delete;
        SimulationModelStatePair& operator=(SimulationModelStatePair&&) noexcept;
        ~SimulationModelStatePair() noexcept;

        std::shared_ptr<Simulation> updSimulation();
        void setSimulation(std::shared_ptr<Simulation>);

        SimulationReport getSimulationReport() const;
        void setSimulationReport(SimulationReport);

    private:
        const OpenSim::Model& implGetModel() const final;
        UID implGetModelVersion() const final;

        const SimTK::State& implGetState() const final;
        UID implGetStateVersion() const final;

        const OpenSim::Component* implGetSelected() const final;
        void implSetSelected(const OpenSim::Component*) final;

        const OpenSim::Component* implGetHovered() const final;
        void implSetHovered(const OpenSim::Component*) final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        std::shared_ptr<Environment> implUpdAssociatedEnvironment() const final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

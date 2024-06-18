#pragma once

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <oscar/Utils/UID.h>

#include <memory>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
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

        OpenSim::Component const* implGetSelected() const final;
        void implSetSelected(OpenSim::Component const*) final;

        OpenSim::Component const* implGetHovered() const final;
        void implSetHovered(OpenSim::Component const*) final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

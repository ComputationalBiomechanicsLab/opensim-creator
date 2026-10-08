#pragma once

#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>

#include <liboscar/utilities/uid.h>

#include <memory>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace osc { class Environment; }
namespace osc { class Simulation; }
namespace SimTK { class State; }

namespace osc
{
    // a readonly model+state pair from a particular step from a simulator
    class SimulationModelStatePair final : public ModelStatePairWithSharedEnvironment {
    public:
        SimulationModelStatePair();
        SimulationModelStatePair(std::shared_ptr<Simulation>, SimulationReport);
        SimulationModelStatePair(const SimulationModelStatePair&) = delete;
        SimulationModelStatePair(SimulationModelStatePair&&) noexcept;
        SimulationModelStatePair& operator=(const SimulationModelStatePair&) = delete;
        SimulationModelStatePair& operator=(SimulationModelStatePair&&) noexcept;
        ~SimulationModelStatePair() noexcept override;

        std::shared_ptr<Simulation> updSimulation();
        void setSimulation(std::shared_ptr<Simulation>);

        SimulationReport getSimulationReport() const;
        void setSimulationReport(SimulationReport);

    private:
        const OpenSim::Model& impl_get_model() const final;
        UID impl_get_model_version() const final;

        const SimTK::State& impl_get_state() const final;
        UID impl_get_state_version() const final;

        const OpenSim::Component* impl_get_selected() const final;
        void impl_set_selected(const OpenSim::Component*) final;

        const OpenSim::Component* impl_get_hovered() const final;
        void impl_set_hovered(const OpenSim::Component*) final;

        float impl_get_fixup_scale_factor() const final;
        void impl_set_fixup_scale_factor(float) final;

        std::shared_ptr<Environment> implUpdAssociatedEnvironment() const final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

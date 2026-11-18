#pragma once

#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>

#include <liboscar/utilities/uid.h>

#include <memory>

namespace OpenSim
{
    class Component;
}

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
    class Simulation;

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

        std::shared_ptr<Simulation> upd_simulation();
        void set_simulation(std::shared_ptr<Simulation>);

        SimulationReport get_simulation_report() const;
        void set_simulation_report(SimulationReport);

    private:
        const OpenSim::Model& impl_get_model() const override;
        UID impl_get_model_version() const override;

        const SimTK::State& impl_get_state() const override;
        UID impl_get_state_version() const override;

        const OpenSim::Component* impl_get_selected() const override;
        void impl_set_selected(const OpenSim::Component*) override;

        const OpenSim::Component* impl_get_hovered() const override;
        void impl_set_hovered(const OpenSim::Component*) override;

        float impl_get_fixup_scale_factor() const override;
        void impl_set_fixup_scale_factor(float) override;

        std::shared_ptr<Environment> impl_upd_associated_environment() const override;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

#pragma once

#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/VirtualModelStatePair.hpp"
#include "src/Utils/UID.hpp"

#include <memory>

namespace OpenSim
{
	class Component;
	class Model;
}

namespace SimTK
{
	class State;
}

namespace osc
{
	// a readonly model+state pair from a particular step from a simulator
	class SimulationModelStatePair : public VirtualModelStatePair {
	public:
		SimulationModelStatePair(std::shared_ptr<Simulation>, SimulationReport);
		SimulationModelStatePair(SimulationModelStatePair const&) = delete;
		SimulationModelStatePair(SimulationModelStatePair&&) noexcept;
		SimulationModelStatePair& operator=(SimulationModelStatePair const&) = delete;
		SimulationModelStatePair& operator=(SimulationModelStatePair&&) noexcept;
		~SimulationModelStatePair() noexcept;

		OpenSim::Model const& getModel() const override;
		OpenSim::Model& updModel() override;  // throws
		UID getModelVersion() const override;

		SimTK::State const& getState() const override;
		UID getStateVersion() const override;

		OpenSim::Component const* getSelected() const override;
		OpenSim::Component* updSelected() override;  // throws
		void setSelected(OpenSim::Component const*) override;

		OpenSim::Component const* getHovered() const override;
		void setHovered(OpenSim::Component const*) override;

		OpenSim::Component const* getIsolated() const override;
		void setIsolated(OpenSim::Component const*) override;

		float getFixupScaleFactor() const override;
		void setFixupScaleFactor(float) override;

		std::shared_ptr<Simulation> updSimulation();
		void setSimulation(std::shared_ptr<Simulation>);

		SimulationReport getSimulationReport() const;
		void setSimulationReport(SimulationReport);

	private:
		class Impl;
		std::unique_ptr<Impl> m_Impl;
	};
}
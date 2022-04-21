#include "SimulatorModelStatePair.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/Utils/UID.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <utility>

class osc::SimulatorModelStatePair::Impl final {
public:
	Impl(std::shared_ptr<Simulation> simulation,
		 SimulationReport simulationReport,
		 float fixupScaleFactor) :
		m_Simulation{std::move(simulation)},
		m_SimulationReport{std::move(simulationReport)},
		m_FixupScaleFactor{std::move(fixupScaleFactor)}
	{
	}

	OpenSim::Model const& getModel() const
	{
		return *m_Simulation->getModel();  // TODO: UH OH - lock leak
	}

	OpenSim::Model& updModel()
	{
		throw std::runtime_error{"cannot update simulator model"};
	}

	UID getModelVersion() const
	{
		return m_ModelVersion;
	}

	SimTK::State const& getState() const
	{
		return m_SimulationReport.getState();
	}

	UID getStateVersion() const
	{
		return m_StateVersion;
	}

	OpenSim::Component const* getSelected() const
	{
		return FindComponent(getModel(), m_Selected);
	}

	OpenSim::Component* updSelected()
	{
		throw std::runtime_error{"cannot update simulator model's selection"};
	}

	void setSelected(OpenSim::Component const* c)
	{
		if (c)
		{
			m_Selected = c->getAbsolutePath();
		}
		else
		{
			m_Selected = {};
		}
	}

	OpenSim::Component const* getHovered() const
	{
		return FindComponent(getModel(), m_Hovered);
	}

	OpenSim::Component* updHovered()
	{
		throw std::runtime_error{"cannot update simulator model's hover"};
	}

	void setHovered(OpenSim::Component const* c)
	{
		if (c)
		{
			m_Hovered = c->getAbsolutePath();
		}
		else
		{
			m_Hovered = {};
		}
	}

	OpenSim::Component const* getIsolated() const
	{
		return FindComponent(getModel(), m_Isolated);
	}

	OpenSim::Component* updIsolated()
	{
		throw std::runtime_error{"cannot update simulator model's isolated"};
	}

	void setIsolated(OpenSim::Component const* c)
	{
		if (c)
		{
			m_Isolated = c->getAbsolutePath();
		}
		else
		{
			m_Isolated = {};
		}
	}

	float getFixupScaleFactor() const
	{
		return m_FixupScaleFactor;
	}

	void setFixupScaleFactor(float v)
	{
		m_FixupScaleFactor = v;
	}

	void setSimulation(std::shared_ptr<Simulation> s)
	{
		if (s != m_Simulation)
		{
			m_Simulation = std::move(s);
			m_ModelVersion = UID{};
		}
	}
	void setSimulationReport(SimulationReport r)
	{
		if (r != m_SimulationReport)
		{
			m_SimulationReport = std::move(r);
			m_StateVersion = UID{};
		}
	}

private:
	UID m_ModelVersion;
	UID m_StateVersion;
	OpenSim::ComponentPath m_Selected;
	OpenSim::ComponentPath m_Hovered;
	OpenSim::ComponentPath m_Isolated;
	std::shared_ptr<Simulation> m_Simulation;
	SimulationReport m_SimulationReport;
	float m_FixupScaleFactor;
};

osc::SimulatorModelStatePair::SimulatorModelStatePair(std::shared_ptr<Simulation> simulation, SimulationReport report, float fixupScaleFactor) :
	m_Impl{std::make_unique<Impl>(std::move(simulation), std::move(report), std::move(fixupScaleFactor))}
{
}
osc::SimulatorModelStatePair::SimulatorModelStatePair(SimulatorModelStatePair&&) noexcept = default;
osc::SimulatorModelStatePair& osc::SimulatorModelStatePair::operator=(SimulatorModelStatePair&&) noexcept = default;
osc::SimulatorModelStatePair::~SimulatorModelStatePair() noexcept = default;

OpenSim::Model const& osc::SimulatorModelStatePair::getModel() const
{
	return m_Impl->getModel();
}

OpenSim::Model& osc::SimulatorModelStatePair::updModel()
{
	return m_Impl->updModel();
}

osc::UID osc::SimulatorModelStatePair::getModelVersion() const
{
	return m_Impl->getModelVersion();
}

SimTK::State const& osc::SimulatorModelStatePair::getState() const
{
	return m_Impl->getState();
}

osc::UID osc::SimulatorModelStatePair::getStateVersion() const
{
	return m_Impl->getStateVersion();
}

OpenSim::Component const* osc::SimulatorModelStatePair::getSelected() const
{
	return m_Impl->getSelected();
}

OpenSim::Component* osc::SimulatorModelStatePair::updSelected()
{
	return m_Impl->updSelected();
}

void osc::SimulatorModelStatePair::setSelected(OpenSim::Component const* c)
{
	m_Impl->setSelected(std::move(c));
}

OpenSim::Component const* osc::SimulatorModelStatePair::getHovered() const
{
	return m_Impl->getHovered();
}

OpenSim::Component* osc::SimulatorModelStatePair::updHovered()
{
	return m_Impl->updHovered();
}

void osc::SimulatorModelStatePair::setHovered(OpenSim::Component const* c)
{
	m_Impl->setHovered(std::move(c));
}

OpenSim::Component const* osc::SimulatorModelStatePair::getIsolated() const
{
	return m_Impl->getIsolated();
}

OpenSim::Component* osc::SimulatorModelStatePair::updIsolated()
{
	return m_Impl->updIsolated();
}

void osc::SimulatorModelStatePair::setIsolated(OpenSim::Component const* c)
{
	m_Impl->setIsolated(std::move(c));
}

float osc::SimulatorModelStatePair::getFixupScaleFactor() const
{
	return m_Impl->getFixupScaleFactor();
}

void osc::SimulatorModelStatePair::setFixupScaleFactor(float v)
{
	m_Impl->setFixupScaleFactor(std::move(v));
}

void osc::SimulatorModelStatePair::setSimulation(std::shared_ptr<Simulation> sim)
{
	m_Impl->setSimulation(std::move(sim));
}

void osc::SimulatorModelStatePair::setSimulationReport(SimulationReport report)
{
	m_Impl->setSimulationReport(std::move(report));
}
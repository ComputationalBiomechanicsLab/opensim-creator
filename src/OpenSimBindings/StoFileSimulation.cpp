#include "StoFileSimulation.hpp"

#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Utils/ScopeGuard.hpp"

#include <nonstd/span.hpp>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/StatesTrajectory.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <vector>

static std::vector<OpenSim::Coordinate*> GetLockedCoordinates(OpenSim::Model& m)
{
	std::vector<OpenSim::Coordinate*> rv;
	for (OpenSim::Coordinate& c : m.updComponentList<OpenSim::Coordinate>())
	{
		if (c.getDefaultLocked())
		{
			rv.push_back(&c);
		}
	}
	return rv;
}

static void SetCoordDefaultLocked(nonstd::span<OpenSim::Coordinate*> cs, bool v)
{
	for (OpenSim::Coordinate* c : cs)
	{
		c->setDefaultLocked(v);
	}
}

static std::vector<osc::SimulationReport> ExtractReports(
	OpenSim::Model& model,
	std::filesystem::path stoFilePath)
{
	std::vector<osc::SimulationReport> rv;

	OpenSim::Storage storage{stoFilePath.string()};

	if (storage.isInDegrees())
	{
		model.getSimbodyEngine().convertDegreesToRadians(storage);
	}

	// HACK: temporarily unlock any coordinates
	std::vector<OpenSim::Coordinate*> lockedCoords = GetLockedCoordinates(model);
	SetCoordDefaultLocked(lockedCoords, false);
	OSC_SCOPE_GUARD({ SetCoordDefaultLocked(lockedCoords, true); });

	OpenSim::StatesTrajectory trajectories =
		OpenSim::StatesTrajectory::createFromStatesStorage(model, storage);

	for (SimTK::State const& s : trajectories)
	{
		rv.emplace_back(model, s);
	}

	return rv;
}

class osc::StoFileSimulation::Impl final {
public:
	Impl(std::unique_ptr<OpenSim::Model> model, std::filesystem::path stoFilePath) :
		m_Model{std::move(model)},
		m_SimulationReports{ExtractReports(*m_Model, stoFilePath)}
	{
	}

	OpenSim::Model const& getModel() const
	{
		return *m_Model;
	}

	int getNumReports() const
	{
		return static_cast<int>(m_SimulationReports.size());
	}

	SimulationReport getSimulationReport(int reportIndex) const
	{
		return m_SimulationReports.at(reportIndex);
	}

	std::vector<SimulationReport> getAllSimulationReports() const
	{
		return m_SimulationReports;
	}

	SimulationStatus getStatus() const
	{
		return SimulationStatus::Completed;
	}

	SimulationClock::time_point getCurTime() const
	{
		return m_End;
	}

	SimulationClock::time_point getStartTime() const
	{
		return m_Start;
	}

	SimulationClock::time_point getEndTime() const
	{
		return m_End;
	}

	float getProgress() const
	{
		return 1.0f;
	}

	ParamBlock const& getParams() const
	{
		return m_ParamBlock;
	}

	nonstd::span<Output const> getOutputs() const
	{
		return {};
	}

	void requestStop()
	{
		// N/A: it's never a "live" sim
	}

	void stop()
	{
		// N/A: it's never a "live" sim
	}

private:
	std::unique_ptr<OpenSim::Model> m_Model;
	std::vector<SimulationReport> m_SimulationReports;
	SimulationClock::time_point m_Start = m_SimulationReports.empty() ? SimulationClock::start() : m_SimulationReports.front().getTime();
	SimulationClock::time_point m_End = m_SimulationReports.empty() ? SimulationClock::start() : m_SimulationReports.back().getTime();
	ParamBlock m_ParamBlock;
};

osc::StoFileSimulation::StoFileSimulation(std::unique_ptr<OpenSim::Model> model, std::filesystem::path stoFilePath) :
	m_Impl{std::make_unique<Impl>(std::move(model), std::move(stoFilePath))}
{
}
osc::StoFileSimulation::StoFileSimulation(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation& osc::StoFileSimulation::operator=(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation::~StoFileSimulation() noexcept = default;

OpenSim::Model const& osc::StoFileSimulation::getModel() const
{
	return m_Impl->getModel();
}

int osc::StoFileSimulation::getNumReports() const
{
	return m_Impl->getNumReports();
}

osc::SimulationReport osc::StoFileSimulation::getSimulationReport(int reportIndex) const
{
	return m_Impl->getSimulationReport(std::move(reportIndex));
}
std::vector<osc::SimulationReport> osc::StoFileSimulation::getAllSimulationReports() const
{
	return m_Impl->getAllSimulationReports();
}

osc::SimulationStatus osc::StoFileSimulation::getStatus() const
{
	return m_Impl->getStatus();
}

osc::SimulationClock::time_point osc::StoFileSimulation::getCurTime() const
{
	return m_Impl->getCurTime();
}

osc::SimulationClock::time_point osc::StoFileSimulation::getStartTime() const
{
	return m_Impl->getStartTime();
}

osc::SimulationClock::time_point osc::StoFileSimulation::getEndTime() const
{
	return m_Impl->getEndTime();
}

float osc::StoFileSimulation::getProgress() const
{
	return m_Impl->getProgress();
}

osc::ParamBlock const& osc::StoFileSimulation::getParams() const
{
	return m_Impl->getParams();
}

nonstd::span<osc::Output const> osc::StoFileSimulation::getOutputs() const
{
	return m_Impl->getOutputs();
}

void osc::StoFileSimulation::requestStop()
{
	m_Impl->requestStop();
}

void osc::StoFileSimulation::stop()
{
	m_Impl->stop();
}

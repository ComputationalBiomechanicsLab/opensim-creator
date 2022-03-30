#include "StoFileSimulation.hpp"

#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
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

static std::vector<int> CreateStorageIndexToModelSvIndexLUT(OpenSim::Model const& model,
                                                            OpenSim::Storage const& storage)
{
	std::vector<int> rv;

	if (storage.getColumnLabels().size() <= 1)
	{
		return rv;
	}

	if (osc::ToLower(storage.getColumnLabels()[0]) != "time")
	{
		return rv;
	}

	struct VarMapping {
		int storageIdx = -1;
		int modelIdx = -1;

		VarMapping(int storageIdx_) : storageIdx{storageIdx_} {}
	};

	std::unordered_map<std::string, VarMapping> mappings;

	// populate mappings LUT with storage indices
	{
		OpenSim::Array<std::string> const& storageLabels = storage.getColumnLabels();

		mappings.reserve(static_cast<size_t>(storageLabels.size()-1));  // skip time col

		for (int i = 1; i < storageLabels.size(); ++i)
		{
			mappings.try_emplace(storageLabels[i], i-1);
		}
	}

	// write model indices into mapping table
	{
		OpenSim::Array<std::string> svNames = model.getStateVariableNames();

		for (int i = 0; i < svNames.size(); ++i)
		{
			auto it = mappings.find(svNames[i]);
			if (it != mappings.end())
			{
				it->second.modelIdx = i;
			}
			else
			{
				osc::log::info("missing statevar: %s", svNames[i].c_str());
			}
		}
	}

	rv.resize(mappings.size(), -1);
	for (auto const& [sv, mapping] : mappings)
	{
		rv[mapping.storageIdx] = mapping.modelIdx;
	}

	OSC_ASSERT(std::none_of(rv.begin(), rv.end(), [](int v) { return v == -1; }));

	return rv;
}

static std::vector<osc::SimulationReport> ExtractReports(
	OpenSim::Model& model,
	std::filesystem::path stoFilePath)
{
	OpenSim::Storage storage{stoFilePath.string()};

	if (storage.isInDegrees())
	{
		model.getSimbodyEngine().convertDegreesToRadians(storage);
	}

    storage.resampleLinear(1.0/100.0);  // TODO: some files can contain thousands of micro-sampled states from OpenSim-GUI

	std::vector<int> lut =
		CreateStorageIndexToModelSvIndexLUT(model, storage);

	// temporarily unlock coords
	std::vector<OpenSim::Coordinate*> lockedCoords = GetLockedCoordinates(model);
	SetCoordDefaultLocked(lockedCoords, false);
	OSC_SCOPE_GUARD({ SetCoordDefaultLocked(lockedCoords, true); });

	// swap space for state vals
	SimTK::Vector stateValsBuf(static_cast<int>(lut.size()), SimTK::NaN);
	SimTK::State st = model.initSystem();
	st.updY().setToNaN();

	std::vector<osc::SimulationReport> rv;
	rv.reserve(storage.getSize());

	for (int row = 0; row < storage.getSize(); ++row)
	{
		OpenSim::StateVector* sv = storage.getStateVector(row);

		st.setTime(sv->getTime());

		OpenSim::Array<double> const& cols = sv->getData();

		OSC_ASSERT_ALWAYS(cols.size() <= static_cast<int>(lut.size()));

		for (int col = 0; col < cols.size(); ++col)
		{
			stateValsBuf[lut[col]] = cols[col];
		}

		model.setStateVariableValues(st, stateValsBuf);
		rv.emplace_back(model, st);
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

    SynchronizedValueGuard<OpenSim::Model const> getModel() const
	{
        return {m_ModelMutex, *m_Model};
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

    nonstd::span<OutputExtractor const> getOutputExtractors() const
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
    mutable std::mutex m_ModelMutex;
    std::unique_ptr<OpenSim::Model> m_Model;
	std::vector<SimulationReport> m_SimulationReports;
	SimulationClock::time_point m_Start = m_SimulationReports.empty() ? SimulationClock::start() : m_SimulationReports.front().getTime();
	SimulationClock::time_point m_End = m_SimulationReports.empty() ? SimulationClock::start() : m_SimulationReports.back().getTime();
	ParamBlock m_ParamBlock;
};

osc::StoFileSimulation::StoFileSimulation(std::unique_ptr<OpenSim::Model> model,
                                          std::filesystem::path stoFilePath) :
	m_Impl{std::make_unique<Impl>(std::move(model), std::move(stoFilePath))}
{
}
osc::StoFileSimulation::StoFileSimulation(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation& osc::StoFileSimulation::operator=(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation::~StoFileSimulation() noexcept = default;

osc::SynchronizedValueGuard<OpenSim::Model const> osc::StoFileSimulation::getModel() const
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

nonstd::span<osc::OutputExtractor const> osc::StoFileSimulation::getOutputExtractors() const
{
    return m_Impl->getOutputExtractors();
}

void osc::StoFileSimulation::requestStop()
{
	m_Impl->requestStop();
}

void osc::StoFileSimulation::stop()
{
	m_Impl->stop();
}

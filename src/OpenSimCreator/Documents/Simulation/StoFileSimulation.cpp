#include "StoFileSimulation.h"

#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Documents/Simulation/SimulationStatus.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/ParamBlock.h>

#include <SimTKcommon.h>
#include <OpenSim/Common/Array.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/StateVector.h>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/SimbodyEngine.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/ScopeGuard.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    SimTK::State CreateStateFromStorageRow(
        OpenSim::Model& model,
        const OpenSim::Storage& storage,
        const std::unordered_map<int, int>& lut,
        int row)
    {
        SimTK::State state = model.getWorkingState();
        UpdateStateVariablesFromStorageRow(model, state, lut, storage, row);
        model.assemble(state);
        model.realizeReport(state);
        return state;
    }

    std::vector<SimulationReport> ExtractReports(
        OpenSim::Model& model,
        const std::filesystem::path& stoFilePath)
    {
        // load+condition the underlying `OpenSim::Storage`
        const auto storage = LoadStorage(model, stoFilePath, StorageLoadingParameters{
            .resampleToFrequency = 1.0/100.0,  // resample the state trajectory at 100FPS (#708)
        });

        // map column header indices in the `OpenSim::Storage` to the `OpenSim::Model`'s state variables' indices
        const auto lut = CreateStorageIndexToModelStatevarMappingWithWarnings(model, *storage);

        // init the model+state with unlocked coordinates
        InitializeModel(model);
        InitializeState(model);

        // convert each timestep in the `OpenSim::Storage` as a `SimulationReport`
        std::vector<SimulationReport> rv;
        rv.reserve(storage->getSize());
        for (int row = 0; row < storage->getSize(); ++row) {
            rv.emplace_back(CreateStateFromStorageRow(model, *storage, lut, row));
        }
        return rv;
    }
}

class osc::StoFileSimulation::Impl final {
public:
    Impl(
        std::unique_ptr<OpenSim::Model> model,
        const std::filesystem::path& stoFilePath,
        float fixupScaleFactor) :

        m_Model{std::move(model)},
        m_SimulationReports{ExtractReports(*m_Model, stoFilePath)},
        m_FixupScaleFactor{fixupScaleFactor}
    {}

    SynchronizedValueGuard<const OpenSim::Model> getModel() const
    {
        return {m_ModelMutex, *m_Model};
    }

    size_t getNumReports() const
    {
        return m_SimulationReports.size();
    }

    SimulationReport getSimulationReport(ptrdiff_t reportIndex) const
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

    SimulationClocks getClocks() const
    {
        return SimulationClocks{{m_Start, m_End}};
    }

    const ParamBlock& getParams() const
    {
        return m_ParamBlock;
    }

    std::span<const OutputExtractor> getOutputExtractors() const
    {
        return {};
    }

    float getFixupScaleFactor() const
    {
        return m_FixupScaleFactor;
    }

    void setFixupScaleFactor(float v)
    {
        m_FixupScaleFactor = v;
    }

private:
    mutable std::mutex m_ModelMutex;
    std::unique_ptr<OpenSim::Model> m_Model;
    std::vector<SimulationReport> m_SimulationReports;
    SimulationClock::time_point m_Start = m_SimulationReports.empty() ? SimulationClock::start() : m_SimulationReports.front().getTime();
    SimulationClock::time_point m_End = m_SimulationReports.empty() ? SimulationClock::start() : m_SimulationReports.back().getTime();
    ParamBlock m_ParamBlock;
    float m_FixupScaleFactor = 1.0f;
};

osc::StoFileSimulation::StoFileSimulation(std::unique_ptr<OpenSim::Model> model, const std::filesystem::path& stoFilePath, float fixupScaleFactor) :
    m_Impl{std::make_unique<Impl>(std::move(model), stoFilePath, fixupScaleFactor)}
{}
osc::StoFileSimulation::StoFileSimulation(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation& osc::StoFileSimulation::operator=(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation::~StoFileSimulation() noexcept = default;

SynchronizedValueGuard<const OpenSim::Model> osc::StoFileSimulation::implGetModel() const
{
    return m_Impl->getModel();
}

ptrdiff_t osc::StoFileSimulation::implGetNumReports() const
{
    return m_Impl->getNumReports();
}

SimulationReport osc::StoFileSimulation::implGetSimulationReport(ptrdiff_t reportIndex) const
{
    return m_Impl->getSimulationReport(reportIndex);
}

std::vector<SimulationReport> osc::StoFileSimulation::implGetAllSimulationReports() const
{
    return m_Impl->getAllSimulationReports();
}

SimulationStatus osc::StoFileSimulation::implGetStatus() const
{
    return m_Impl->getStatus();
}

SimulationClocks osc::StoFileSimulation::implGetClocks() const
{
    return m_Impl->getClocks();
}

const ParamBlock& osc::StoFileSimulation::implGetParams() const
{
    return m_Impl->getParams();
}

std::span<const OutputExtractor> osc::StoFileSimulation::implGetOutputExtractors() const
{
    return m_Impl->getOutputExtractors();
}

float osc::StoFileSimulation::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::StoFileSimulation::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}

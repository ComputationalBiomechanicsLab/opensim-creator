#include "StoFileSimulation.hpp"

#include "OpenSimCreator/Simulation/SimulationClock.hpp"
#include "OpenSimCreator/Simulation/SimulationReport.hpp"
#include "OpenSimCreator/Simulation/SimulationStatus.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Utils/ParamBlock.hpp"

#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/ScopeGuard.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <nonstd/span.hpp>
#include <OpenSim/Common/Array.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/StateVector.h>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Common/TableUtilities.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/SimbodyEngine.h>
#include <SimTKcommon.h>
#include <SimTKcommon/Orientation.h>
#include <SimTKcommon/Scalar.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
    std::vector<OpenSim::Coordinate*> GetLockedCoordinates(OpenSim::Model& m)
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

    void SetCoordsDefaultLocked(nonstd::span<OpenSim::Coordinate*> cs, bool v)
    {
        for (OpenSim::Coordinate* c : cs)
        {
            c->setDefaultLocked(v);
        }
    }

    template<typename T>
    std::unordered_set<T> ToSet(OpenSim::Array<T> const& v)
    {
        std::unordered_set<T> rv;
        for (int i = 0; i < v.size(); ++i)
        {
            rv.insert(v[i]);
        }
        return rv;
    }

    template<typename T>
    int NumUniqueEntriesIn(OpenSim::Array<T> const& v)
    {
        return static_cast<int>(ToSet(v).size());
    }

    template<typename T>
    bool AllElementsUnique(OpenSim::Array<T> const& v)
    {
        return NumUniqueEntriesIn(v) == v.size();
    }

    std::unordered_map<int, int> CreateStorageIndexToModelSvIndexLUT(
        OpenSim::Model const& model,
        OpenSim::Storage const& storage)
    {
        std::unordered_map<int, int> rv;

        if (storage.getColumnLabels().size() <= 1)
        {
            osc::log::warn("the provided STO file does not contain any state variable data");
            return rv;
        }

        if (!osc::IsEqualCaseInsensitive(storage.getColumnLabels()[0], "time"))
        {
            throw std::runtime_error{"the provided STO file does not contain a 'time' column as its first column: it cannot be processed"};
        }

        // care: the storage column labels do not match the state variable names
        // in the model 1:1
        //
        // STO files have changed over time. Pre-4.0 uses different naming conventions
        // etc for the column labels, so you *need* to map the storage column strings
        // carefully onto the model statevars
        std::vector<std::string> missing;
        OpenSim::Array<std::string> const& storageColumnsIncludingTime = storage.getColumnLabels();
        OpenSim::Array<std::string> modelStateVars = model.getStateVariableNames();

        if (!AllElementsUnique(storageColumnsIncludingTime))
        {
            throw std::runtime_error{"the provided STO file contains multiple columns with the same name. This creates ambiguities, which OSC can't handle"};
        }

        // compute mapping
        rv.reserve(modelStateVars.size());
        for (int modelIndex = 0; modelIndex < modelStateVars.size(); ++modelIndex)
        {
            std::string const& svName = modelStateVars[modelIndex];
            int storageIndex = OpenSim::TableUtilities::findStateLabelIndex(storageColumnsIncludingTime, svName);
            int valueIndex = storageIndex - 1;  // the column labels include 'time', which isn't in the data elements

            if (valueIndex >= 0)
            {
                rv[valueIndex] = modelIndex;
            }
            else
            {
                missing.push_back(svName);
            }
        }

        // ensure all model state variables are accounted for
        if (!missing.empty())
        {
            std::stringstream ss;
            ss << "the provided STO file is missing the following columns:\n";
            std::string_view delim;
            for (std::string const& el : missing)
            {
                ss << delim << el;
                delim = ", ";
            }
            osc::log::warn("%s", std::move(ss).str().c_str());
            osc::log::warn("The STO file was loaded successfully, but beware: the missing state variables have been defaulted in order for this to work");
            osc::log::warn("Therefore, do not treat the motion you are seeing as a 'true' representation of something: some state data was 'made up' to make the motion viewable");
        }

        // else: all model state variables are accounted for

        return rv;
    }

    std::vector<osc::SimulationReport> ExtractReports(
        OpenSim::Model& model,
        std::filesystem::path const& stoFilePath)
    {
        OpenSim::Storage storage{stoFilePath.string()};

        if (storage.isInDegrees())
        {
            model.getSimbodyEngine().convertDegreesToRadians(storage);
        }

        storage.resampleLinear(1.0/100.0);  // #708

        std::unordered_map<int, int> lut =
            CreateStorageIndexToModelSvIndexLUT(model, storage);

        // temporarily unlock coords
        std::vector<OpenSim::Coordinate*> lockedCoords = GetLockedCoordinates(model);
        SetCoordsDefaultLocked(lockedCoords, false);
        OSC_SCOPE_GUARD({ SetCoordsDefaultLocked(lockedCoords, true); });

        osc::InitializeModel(model);
        osc::InitializeState(model);

        std::vector<osc::SimulationReport> rv;
        rv.reserve(storage.getSize());

        for (int row = 0; row < storage.getSize(); ++row)
        {
            OpenSim::StateVector* sv = storage.getStateVector(row);
            OpenSim::Array<double> const& cols = sv->getData();

            SimTK::Vector stateValsBuf = model.getStateVariableValues(model.getWorkingState());
            for (auto [valueIdx, modelIdx] : lut)
            {
                if (0 <= valueIdx && valueIdx < cols.size() && 0 <= modelIdx && modelIdx < stateValsBuf.size())
                {
                    stateValsBuf[modelIdx] = cols[valueIdx];
                }
                else
                {
                    throw std::runtime_error{"an index in the stroage lookup was invalid: this is probably a developer error that needs to be investigated (report it)"};
                }
            }

            osc::SimulationReport& report = rv.emplace_back(SimTK::State{model.getWorkingState()});
            SimTK::State& st = report.updStateHACK();
            st.setTime(sv->getTime());
            model.setStateVariableValues(st, stateValsBuf);
            model.realizeReport(st);
        }

        return rv;
    }
}

class osc::StoFileSimulation::Impl final {
public:
    Impl(
        std::unique_ptr<OpenSim::Model> model,
        std::filesystem::path const& stoFilePath,
        float fixupScaleFactor) :

        m_Model{std::move(model)},
        m_SimulationReports{ExtractReports(*m_Model, stoFilePath)},
        m_FixupScaleFactor{fixupScaleFactor}
    {
    }

    SynchronizedValueGuard<OpenSim::Model const> getModel() const
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

osc::StoFileSimulation::StoFileSimulation(std::unique_ptr<OpenSim::Model> model, std::filesystem::path const& stoFilePath, float fixupScaleFactor) :
    m_Impl{std::make_unique<Impl>(std::move(model), stoFilePath, fixupScaleFactor)}
{
}

osc::StoFileSimulation::StoFileSimulation(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation& osc::StoFileSimulation::operator=(StoFileSimulation&&) noexcept = default;
osc::StoFileSimulation::~StoFileSimulation() noexcept = default;

osc::SynchronizedValueGuard<OpenSim::Model const> osc::StoFileSimulation::implGetModel() const
{
    return m_Impl->getModel();
}

ptrdiff_t osc::StoFileSimulation::implGetNumReports() const
{
    return m_Impl->getNumReports();
}

osc::SimulationReport osc::StoFileSimulation::implGetSimulationReport(ptrdiff_t reportIndex) const
{
    return m_Impl->getSimulationReport(reportIndex);
}

std::vector<osc::SimulationReport> osc::StoFileSimulation::implGetAllSimulationReports() const
{
    return m_Impl->getAllSimulationReports();
}

osc::SimulationStatus osc::StoFileSimulation::implGetStatus() const
{
    return m_Impl->getStatus();
}

osc::SimulationClock::time_point osc::StoFileSimulation::implGetCurTime() const
{
    return m_Impl->getCurTime();
}

osc::SimulationClock::time_point osc::StoFileSimulation::implGetStartTime() const
{
    return m_Impl->getStartTime();
}

osc::SimulationClock::time_point osc::StoFileSimulation::implGetEndTime() const
{
    return m_Impl->getEndTime();
}

float osc::StoFileSimulation::implGetProgress() const
{
    return m_Impl->getProgress();
}

osc::ParamBlock const& osc::StoFileSimulation::implGetParams() const
{
    return m_Impl->getParams();
}

nonstd::span<osc::OutputExtractor const> osc::StoFileSimulation::implGetOutputExtractors() const
{
    return m_Impl->getOutputExtractors();
}

void osc::StoFileSimulation::implRequestStop()
{
    m_Impl->requestStop();
}

void osc::StoFileSimulation::implStop()
{
    m_Impl->stop();
}

float osc::StoFileSimulation::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::StoFileSimulation::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}

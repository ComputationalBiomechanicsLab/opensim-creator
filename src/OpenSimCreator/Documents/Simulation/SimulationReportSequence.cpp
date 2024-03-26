#include "SimulationReportSequence.h"

#include <OpenSimCreator/Documents/Simulation/AuxiliaryValue.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReportSequenceCursor.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CopyOnUpdPtr.h>
#include <SimTKcommon.h>

#include <cstddef>
#include <iterator>
#include <span>
#include <vector>

using namespace osc;

namespace
{
    SimulationClock::time_point time_of(SimTK::State const& state)
    {
        return SimulationClock::start() + SimulationClock::duration{state.getTime()};
    }
}

class osc::SimulationReportSequence::Impl final {
public:
    size_t size() const { return m_States.size(); }

    SimulationClock::time_point frontTime() const
    {
        return size() > 0 ? timeOf(0) : SimulationClock::start();
    }

    SimulationClock::time_point backTime() const
    {
        return size() > 0 ? timeOf(size() - 1) : SimulationClock::start();
    }

    std::optional<SimulationClock::time_point> prevTime(SimulationClock::time_point t) const
    {
        return timeOfStateAfterOrIncluding(t, -1);
    }

    std::optional<SimulationClock::time_point> nextTime(SimulationClock::time_point t) const
    {
        return timeOfStateAfterOrIncluding(t, 1);
    }

    std::optional<size_t> indexOfStateAfterOrIncluding(SimulationClock::time_point t, int offset = 0) const
    {
        ptrdiff_t const n = ssize(m_States);

        if (n == 0) {
            return std::nullopt;
        }

        ptrdiff_t zeroethReportIndex = n - 1;
        for (ptrdiff_t i = 0; i < n; ++i) {
            if (timeOf(i) >= t) {
                zeroethReportIndex = i;
                break;
            }
        }

        ptrdiff_t const reportIndex = zeroethReportIndex + offset;
        if (0 <= reportIndex && reportIndex < n) {
            return static_cast<size_t>(reportIndex);
        }
        else {
            return std::nullopt;
        }
    }

    void reserve(size_t i)
    {
        m_States.reserve(i);
        m_AuxiliaryValues.reserve(i);
    }

    void emplace_back(SimTK::State&& state, std::span<AuxiliaryValue const> auxiliaryValues)
    {
        m_States.push_back(make_cow<SimTK::State>(std::move(state)));
        m_AuxiliaryValues.push_back(std::vector<AuxiliaryValue>(auxiliaryValues.begin(), auxiliaryValues.end()));
    }

    void seek(SimulationReportSequenceCursor& cursor, OpenSim::Model const&, size_t i) const
    {
        cursor.setState(m_States.at(i));
        cursor.setAuxiliaryVariables(m_AuxiliaryValues.at(i));
    }

    SimulationClock::time_point timeOf(size_t i) const
    {
        return time_of(*m_States.at(i));
    }
private:
    std::optional<SimulationClock::time_point> timeOfStateAfterOrIncluding(SimulationClock::time_point t, int offset = 0) const
    {
        if (auto idx = indexOfStateAfterOrIncluding(t, offset)) {
            return timeOf(*idx);
        }
        else {
            return std::nullopt;
        }
    }

    // TODO: optimize this shit: should only store state variables
    std::vector<CopyOnUpdPtr<SimTK::State>> m_States;
    std::vector<std::vector<AuxiliaryValue>> m_AuxiliaryValues;
};


osc::SimulationReportSequence::SimulationReportSequence() :
    m_Impl{make_cow<Impl>()}
{}

size_t osc::SimulationReportSequence::size() const
{
    return m_Impl->size();
}

SimulationClock::time_point osc::SimulationReportSequence::frontTime() const
{
    return m_Impl->frontTime();
}

SimulationClock::time_point osc::SimulationReportSequence::backTime() const
{
    return m_Impl->backTime();
}

std::optional<SimulationClock::time_point> osc::SimulationReportSequence::prevTime(SimulationClock::time_point t) const
{
    return m_Impl->prevTime(t);
}

std::optional<SimulationClock::time_point> osc::SimulationReportSequence::nextTime(SimulationClock::time_point t) const
{
    return m_Impl->nextTime(t);
}

std::optional<size_t> osc::SimulationReportSequence::indexOfStateAfterOrIncluding(SimulationClock::time_point t) const
{
    return m_Impl->indexOfStateAfterOrIncluding(t);
}

void osc::SimulationReportSequence::reserve(size_t i)
{
    m_Impl.upd()->reserve(i);
}

void osc::SimulationReportSequence::emplace_back(
    SimTK::State&& state,
    std::span<AuxiliaryValue const> auxiliaryValues)
{
    m_Impl.upd()->emplace_back(std::move(state), auxiliaryValues);
}

void osc::SimulationReportSequence::seek(
    SimulationReportSequenceCursor& cursor,
    OpenSim::Model const& model,
    size_t i) const
{
    m_Impl->seek(cursor, model, i);
}

SimulationClock::time_point osc::SimulationReportSequence::timeOf(size_t i) const
{
    return m_Impl->timeOf(i);
}


#include "SimulationReport.h"

#include <SimTKcommon.h>

#include <oscar/Utils/Algorithms.h>

#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>

using namespace osc;

class osc::SimulationReport::Impl final {
public:
    Impl() = default;

    explicit Impl(SimTK::State&& st) :
        m_State{std::move(st)}
    {
    }

    Impl(SimTK::State&& st, std::unordered_map<UID, float> auxiliaryValues) :
        m_State{std::move(st)},
        m_AuxiliaryValues{std::move(auxiliaryValues)}
    {
    }

    std::unique_ptr<Impl> clone() const
    {
        return std::make_unique<Impl>(*this);
    }

    SimulationClock::time_point getTime() const
    {
        return SimulationClock::start() + SimulationClock::duration{m_State.getTime()};
    }

    SimTK::State& updStateHACK()
    {
        return m_State;
    }

    std::optional<float> getAuxiliaryValue(UID id) const
    {
        return find_or_optional(m_AuxiliaryValues, id);
    }

    std::vector<AuxiliaryValue> getAllAuxiliaryValuesHACK() const
    {
        std::vector<AuxiliaryValue> rv;
        rv.reserve(m_AuxiliaryValues.size());
        for (auto const& [id, v] : m_AuxiliaryValues) {
            rv.push_back(AuxiliaryValue{id, v});
        }
        return rv;
    }

private:
    SimTK::State m_State;
    std::unordered_map<UID, float> m_AuxiliaryValues;
};


// public API

osc::SimulationReport::SimulationReport() :
    m_Impl{std::make_shared<Impl>()}
{
}

osc::SimulationReport::SimulationReport(SimTK::State&& st) :
    m_Impl{std::make_shared<Impl>(std::move(st))}
{
}
osc::SimulationReport::SimulationReport(SimTK::State&& st, std::unordered_map<UID, float> auxiliaryValues) :
    m_Impl{std::make_shared<Impl>(std::move(st), std::move(auxiliaryValues))}
{
}
osc::SimulationReport::SimulationReport(SimulationReport const&) = default;
osc::SimulationReport::SimulationReport(SimulationReport&&) noexcept = default;
osc::SimulationReport& osc::SimulationReport::operator=(SimulationReport const&) = default;
osc::SimulationReport& osc::SimulationReport::operator=(SimulationReport&&) noexcept = default;
osc::SimulationReport::~SimulationReport() noexcept = default;

SimulationClock::time_point osc::SimulationReport::getTime() const
{
    return m_Impl->getTime();
}

SimTK::State&& osc::SimulationReport::stealState() &&
{
    return std::move(m_Impl->updStateHACK());
}

std::optional<float> osc::SimulationReport::getAuxiliaryValue(UID id) const
{
    return m_Impl->getAuxiliaryValue(id);
}

std::vector<AuxiliaryValue> osc::SimulationReport::getAllAuxiliaryValuesHACK() const
{
    return m_Impl->getAllAuxiliaryValuesHACK();
}

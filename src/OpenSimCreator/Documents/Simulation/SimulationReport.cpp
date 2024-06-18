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
    {}

    Impl(SimTK::State&& st, std::unordered_map<UID, float> auxiliaryValues) :
        m_State{std::move(st)},
        m_AuxiliaryValues{std::move(auxiliaryValues)}
    {}

    std::unique_ptr<Impl> clone() const
    {
        return std::make_unique<Impl>(*this);
    }

    SimulationClock::time_point getTime() const
    {
        return SimulationClock::start() + SimulationClock::duration{getState().getTime()};
    }

    const SimTK::State& getState() const
    {
        return m_State;
    }

    SimTK::State& updStateHACK()
    {
        return m_State;
    }

    std::optional<float> getAuxiliaryValue(UID id) const
    {
        return lookup_or_nullopt(m_AuxiliaryValues, id);
    }

private:
    SimTK::State m_State;
    std::unordered_map<UID, float> m_AuxiliaryValues;
};


osc::SimulationReport::SimulationReport() :
    m_Impl{std::make_shared<Impl>()}
{}
osc::SimulationReport::SimulationReport(SimTK::State&& st) :
    m_Impl{std::make_shared<Impl>(std::move(st))}
{}
osc::SimulationReport::SimulationReport(SimTK::State&& st, std::unordered_map<UID, float> auxiliaryValues) :
    m_Impl{std::make_shared<Impl>(std::move(st), std::move(auxiliaryValues))}
{}
osc::SimulationReport::SimulationReport(const SimulationReport&) = default;
osc::SimulationReport::SimulationReport(SimulationReport&&) noexcept = default;
osc::SimulationReport& osc::SimulationReport::operator=(const SimulationReport&) = default;
osc::SimulationReport& osc::SimulationReport::operator=(SimulationReport&&) noexcept = default;
osc::SimulationReport::~SimulationReport() noexcept = default;

SimulationClock::time_point osc::SimulationReport::getTime() const
{
    return m_Impl->getTime();
}

const SimTK::State& osc::SimulationReport::getState() const
{
    return m_Impl->getState();
}

SimTK::State& osc::SimulationReport::updStateHACK()
{
    return m_Impl->updStateHACK();
}

std::optional<float> osc::SimulationReport::getAuxiliaryValue(UID id) const
{
    return m_Impl->getAuxiliaryValue(id);
}

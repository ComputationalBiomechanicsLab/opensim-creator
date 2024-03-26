#include "SimulationReportSequenceCursor.h"

#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>

#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CopyOnUpdPtr.h>
#include <oscar/Utils/UID.h>
#include <SimTKcommon.h>

#include <cstddef>
#include <optional>
#include <unordered_map>

using namespace osc;

class osc::SimulationReportSequenceCursor::Impl final {
public:
    SimTK::State const& state() const { return *m_State; }
    std::optional<float> findAuxiliaryValue(UID id) const { return find_or_optional(m_AuxiliaryValues, id); }

    void setState(CopyOnUpdPtr<SimTK::State> const& state) { m_State = state; }

    void setAuxiliaryVariables(std::span<AuxiliaryValue const> vals)
    {
        m_AuxiliaryValues.clear();
        for (auto const& [id, v] : vals) {
            m_AuxiliaryValues[id] = v;
        }
    }
private:
    CopyOnUpdPtr<SimTK::State> m_State = make_cow<SimTK::State>();
    std::unordered_map<UID, float> m_AuxiliaryValues;
};

osc::SimulationReportSequenceCursor::SimulationReportSequenceCursor() :
    m_Impl{make_cow<Impl>()}
{}

SimTK::State const& osc::SimulationReportSequenceCursor::implGetState() const { return m_Impl->state(); }
std::optional<float> osc::SimulationReportSequenceCursor::implFindAuxiliaryValue(UID id) const { return m_Impl->findAuxiliaryValue(id); }

void osc::SimulationReportSequenceCursor::setState(CopyOnUpdPtr<SimTK::State> const& state)
{
    m_Impl.upd()->setState(state);
}

void osc::SimulationReportSequenceCursor::setAuxiliaryVariables(std::span<AuxiliaryValue const> vals)
{
    m_Impl.upd()->setAuxiliaryVariables(vals);
}

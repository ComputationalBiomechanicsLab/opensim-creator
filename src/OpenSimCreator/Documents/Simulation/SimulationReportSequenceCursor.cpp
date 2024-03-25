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
    size_t index() const { return m_Index; }
    SimulationClock::time_point time() const { return SimulationClock::start() + SimulationClock::duration{state().getTime()}; }
    SimTK::State const& state() const { return m_State; }
    std::optional<float> findAuxiliaryValue(UID id) const { return find_or_optional(m_AuxiliaryValues, id); }

    void setIndex(size_t newIndex) { m_Index = newIndex; }
    SimTK::State& updState() { return m_State; }
    void clearAuxiliaryValues() { m_AuxiliaryValues.clear(); }
    void setAuxiliaryValue(AuxiliaryValue v) { m_AuxiliaryValues[v.id] = v.value; }
private:
    size_t m_Index = 0;
    SimTK::State m_State;
    std::unordered_map<UID, float> m_AuxiliaryValues;
};

osc::SimulationReportSequenceCursor::SimulationReportSequenceCursor() :
    m_Impl{make_cow<Impl>()}
{}

size_t osc::SimulationReportSequenceCursor::index() const { return m_Impl->index(); }
SimulationClock::time_point osc::SimulationReportSequenceCursor::time() const { return m_Impl->time(); }
SimTK::State const& osc::SimulationReportSequenceCursor::state() const { return m_Impl->state(); }
std::optional<float> osc::SimulationReportSequenceCursor::findAuxiliaryValue(UID id) const { return m_Impl->findAuxiliaryValue(id); }

void osc::SimulationReportSequenceCursor::setIndex(size_t newIndex) { m_Impl.upd()->setIndex(newIndex); }
SimTK::State& osc::SimulationReportSequenceCursor::updState() { return m_Impl.upd()->updState(); }
void osc::SimulationReportSequenceCursor::clearAuxiliaryValues() { m_Impl.upd()->clearAuxiliaryValues(); }
void osc::SimulationReportSequenceCursor::setAuxiliaryValue(AuxiliaryValue v) { m_Impl.upd()->setAuxiliaryValue(v); }

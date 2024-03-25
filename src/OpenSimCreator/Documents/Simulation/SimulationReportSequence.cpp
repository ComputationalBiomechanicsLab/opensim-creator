#include "SimulationReportSequence.h"

#include <OpenSimCreator/Documents/Simulation/AuxiliaryValue.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReportSequenceCursor.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

#include <cstddef>
#include <span>
#include <vector>

using namespace osc;

class osc::SimulationReportSequence::Impl final {
public:
    size_t size() const { return m_States.size(); }

    void emplace_back(SimTK::State const& state, std::span<AuxiliaryValue const> auxiliaryValues)
    {
        m_States.emplace_back(state);
        m_AuxiliaryValues.push_back(std::vector<AuxiliaryValue>(auxiliaryValues.begin(), auxiliaryValues.end()));
    }

    void seek(SimulationReportSequenceCursor& cursor, OpenSim::Model const& model, size_t i) const
    {
        // update state
        SimTK::State& cursorState = cursor.updState();
        cursorState = m_States.at(i);
        model.realizeReport(cursorState);

        // update aux vars
        cursor.clearAuxiliaryValues();
        for (AuxiliaryValue const& v : m_AuxiliaryValues.at(i)) {
            cursor.setAuxiliaryValue(v);
        }
    }
private:
    // TODO: optimize this shit: should only store state variables
    std::vector<SimTK::State> m_States;
    std::vector<std::vector<AuxiliaryValue>> m_AuxiliaryValues;
};


osc::SimulationReportSequence::SimulationReportSequence() :
    m_Impl{make_cow<Impl>()}
{}

size_t osc::SimulationReportSequence::size() const
{
    return m_Impl->size();
}

void osc::SimulationReportSequence::emplace_back(
    SimTK::State const& state,
    std::span<AuxiliaryValue const> auxiliaryValues)
{
    m_Impl.upd()->emplace_back(state, auxiliaryValues);
}

void osc::SimulationReportSequence::seek(
    SimulationReportSequenceCursor& cursor,
    OpenSim::Model const& model,
    size_t i) const
{
    m_Impl->seek(cursor, model, i);
}

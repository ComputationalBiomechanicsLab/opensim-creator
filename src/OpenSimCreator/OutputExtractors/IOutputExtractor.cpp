#include "IOutputExtractor.h"

#include <OpenSimCreator/OutputExtractors/OutputValueExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReportSequence.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReportSequenceCursor.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Maths/Constants.h>

#include <array>
#include <span>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

float osc::IOutputExtractor::getValueFloat(
    OpenSim::Component const& component,
    ISimulationState const& state) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    return extractor(state).to<float>();
}

void osc::IOutputExtractor::getValuesFloat(
    OpenSim::Model const& model,
    SimulationReportSequence const& reports,
    std::function<void(float)> const& consumer) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(model);

    SimulationReportSequenceCursor cursor;
    for (size_t i = 0; i < reports.size(); ++i) {
        reports.seek(cursor, model, i);
        consumer(extractor(cursor));
    }
}

std::vector<float> osc::IOutputExtractor::slurpValuesFloat(
    OpenSim::Model const& model,
    SimulationReportSequence const& reports) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(model);

    SimulationReportSequenceCursor cursor;
    std::vector<float> rv;
    rv.reserve(reports.size());
    for (size_t i = 0; i < reports.size(); ++i) {
        reports.seek(cursor, model, i);
        rv.push_back(extractor(cursor).to<float>());
    }
    return rv;
}

Vec2 osc::IOutputExtractor::getValueVec2(
    OpenSim::Component const& component,
    ISimulationState const& state) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    return extractor(state).to<Vec2>();
}

void osc::IOutputExtractor::getValuesVec2(
    OpenSim::Model const& model,
    SimulationReportSequence const& reports,
    std::function<void(Vec2)> const& consumer) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(model);

    SimulationReportSequenceCursor cursor;
    for (size_t i = 0; i < reports.size(); ++i) {
        reports.seek(cursor, model, i);
        consumer(extractor(cursor).to<Vec2>());
    }
}

std::vector<Vec2> osc::IOutputExtractor::slurpValuesVec2(
    OpenSim::Model const& model,
    SimulationReportSequence const& reports) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(model);

    SimulationReportSequenceCursor cursor;
    std::vector<Vec2> rv;
    rv.reserve(reports.size());
    for (size_t i = 0; i < reports.size(); ++i) {
        rv.push_back(extractor(cursor).to<Vec2>());
    }
    return rv;
}

std::string osc::IOutputExtractor::getValueString(
    OpenSim::Component const& component,
    ISimulationState const& report) const
{
    return getOutputValueExtractor(component)(report).to<std::string>();
}

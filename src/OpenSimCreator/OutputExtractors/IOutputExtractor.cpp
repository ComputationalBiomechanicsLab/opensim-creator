#include "IOutputExtractor.h"

#include <OpenSimCreator/OutputExtractors/OutputValueExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <oscar/Maths/Constants.h>

#include <array>
#include <span>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

float osc::IOutputExtractor::getValueFloat(
    OpenSim::Component const& component,
    SimulationReport const& report) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    return extractor(report).to<float>();
}

void osc::IOutputExtractor::getValuesFloat(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports,
    std::function<void(float)> const& consumer) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    for (auto const& report : reports) {
        consumer(extractor(report).to<float>());
    }
}

std::vector<float> osc::IOutputExtractor::slurpValuesFloat(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);

    std::vector<float> rv;
    rv.reserve(reports.size());
    for (auto const& report : reports) {
        rv.push_back(extractor(report).to<float>());
    }
    return rv;
}

Vec2 osc::IOutputExtractor::getValueVec2(
    OpenSim::Component const& component,
    SimulationReport const& report) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    return extractor(report).to<Vec2>();
}

void osc::IOutputExtractor::getValuesVec2(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports,
    std::function<void(Vec2)> const& consumer) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    for (auto const& report : reports) {
        consumer(extractor(report).to<Vec2>());
    }
}

std::vector<Vec2> osc::IOutputExtractor::slurpValuesVec2(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);

    std::vector<Vec2> rv;
    rv.reserve(reports.size());
    for (auto const& report : reports) {
        rv.push_back(extractor(report).to<Vec2>());
    }
    return rv;
}

std::string osc::IOutputExtractor::getValueString(
    OpenSim::Component const& component,
    SimulationReport const& report) const
{
    return getOutputValueExtractor(component)(report).to<std::string>();
}

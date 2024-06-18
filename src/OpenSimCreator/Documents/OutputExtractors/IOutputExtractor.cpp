#include "IOutputExtractor.h"

#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <oscar/Maths/Constants.h>

#include <array>
#include <span>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

float osc::IOutputExtractor::getValueFloat(
    const OpenSim::Component& component,
    const SimulationReport& report) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    return extractor(report).to<float>();
}

void osc::IOutputExtractor::getValuesFloat(
    const OpenSim::Component& component,
    std::span<SimulationReport const> reports,
    std::function<void(float)> const& consumer) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    for (const auto& report : reports) {
        consumer(extractor(report).to<float>());
    }
}

std::vector<float> osc::IOutputExtractor::slurpValuesFloat(
    const OpenSim::Component& component,
    std::span<SimulationReport const> reports) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);

    std::vector<float> rv;
    rv.reserve(reports.size());
    for (const auto& report : reports) {
        rv.push_back(extractor(report).to<float>());
    }
    return rv;
}

Vec2 osc::IOutputExtractor::getValueVec2(
    const OpenSim::Component& component,
    const SimulationReport& report) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    return extractor(report).to<Vec2>();
}

void osc::IOutputExtractor::getValuesVec2(
    const OpenSim::Component& component,
    std::span<SimulationReport const> reports,
    std::function<void(Vec2)> const& consumer) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);
    for (const auto& report : reports) {
        consumer(extractor(report).to<Vec2>());
    }
}

std::vector<Vec2> osc::IOutputExtractor::slurpValuesVec2(
    const OpenSim::Component& component,
    std::span<SimulationReport const> reports) const
{
    OutputValueExtractor const extractor = getOutputValueExtractor(component);

    std::vector<Vec2> rv;
    rv.reserve(reports.size());
    for (const auto& report : reports) {
        rv.push_back(extractor(report).to<Vec2>());
    }
    return rv;
}

std::string osc::IOutputExtractor::getValueString(
    const OpenSim::Component& component,
    const SimulationReport& report) const
{
    return getOutputValueExtractor(component)(report).to<std::string>();
}

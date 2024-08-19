#include "IOutputExtractor.h"

#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>

#include <oscar/Maths/Constants.h>
#include <oscar/Utils/Conversion.h>

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
    const OutputValueExtractor extractor = getOutputValueExtractor(component);
    return to<float>(extractor(report));
}

void osc::IOutputExtractor::getValuesFloat(
    const OpenSim::Component& component,
    std::span<const SimulationReport> reports,
    const std::function<void(float)>& consumer) const
{
    const OutputValueExtractor extractor = getOutputValueExtractor(component);
    for (const auto& report : reports) {
        consumer(to<float>(extractor(report)));
    }
}

std::vector<float> osc::IOutputExtractor::slurpValuesFloat(
    const OpenSim::Component& component,
    std::span<const SimulationReport> reports) const
{
    const OutputValueExtractor extractor = getOutputValueExtractor(component);

    std::vector<float> rv;
    rv.reserve(reports.size());
    for (const auto& report : reports) {
        rv.push_back(to<float>(extractor(report)));
    }
    return rv;
}

Vec2 osc::IOutputExtractor::getValueVec2(
    const OpenSim::Component& component,
    const SimulationReport& report) const
{
    const OutputValueExtractor extractor = getOutputValueExtractor(component);
    return to<Vec2>(extractor(report));
}

void osc::IOutputExtractor::getValuesVec2(
    const OpenSim::Component& component,
    std::span<const SimulationReport> reports,
    const std::function<void(Vec2)>& consumer) const
{
    const OutputValueExtractor extractor = getOutputValueExtractor(component);
    for (const auto& report : reports) {
        consumer(to<Vec2>(extractor(report)));
    }
}

std::vector<Vec2> osc::IOutputExtractor::slurpValuesVec2(
    const OpenSim::Component& component,
    std::span<const SimulationReport> reports) const
{
    const OutputValueExtractor extractor = getOutputValueExtractor(component);

    std::vector<Vec2> rv;
    rv.reserve(reports.size());
    for (const auto& report : reports) {
        rv.push_back(to<Vec2>(extractor(report)));
    }
    return rv;
}

std::string osc::IOutputExtractor::getValueString(
    const OpenSim::Component& component,
    const SimulationReport& report) const
{
    return to<std::string>(getOutputValueExtractor(component)(report));
}

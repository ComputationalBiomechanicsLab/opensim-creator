#include "IOutputExtractor.h"

#include <libopensimcreator/Documents/OutputExtractors/OutputValueExtractor.h>
#include <libopensimcreator/Documents/Simulation/SimulationReport.h>

#include <liboscar/utils/Conversion.h>

#include <functional>
#include <span>
#include <string>
#include <vector>

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

Vector2 osc::IOutputExtractor::getValueVector2(
    const OpenSim::Component& component,
    const SimulationReport& report) const
{
    const OutputValueExtractor extractor = getOutputValueExtractor(component);
    return to<Vector2>(extractor(report));
}

void osc::IOutputExtractor::getValuesVector2(
    const OpenSim::Component& component,
    std::span<const SimulationReport> reports,
    const std::function<void(Vector2)>& consumer) const
{
    const OutputValueExtractor extractor = getOutputValueExtractor(component);
    for (const auto& report : reports) {
        consumer(to<Vector2>(extractor(report)));
    }
}

std::vector<Vector2> osc::IOutputExtractor::slurpValuesVector2(
    const OpenSim::Component& component,
    std::span<const SimulationReport> reports) const
{
    const OutputValueExtractor extractor = getOutputValueExtractor(component);

    std::vector<Vector2> rv;
    rv.reserve(reports.size());
    for (const auto& report : reports) {
        rv.push_back(to<Vector2>(extractor(report)));
    }
    return rv;
}

std::string osc::IOutputExtractor::getValueString(
    const OpenSim::Component& component,
    const SimulationReport& report) const
{
    return to<std::string>(getOutputValueExtractor(component)(report));
}

#include "IOutputExtractor.h"

#include <OpenSimCreator/OutputExtractors/IFloatOutputValueExtractor.h>
#include <OpenSimCreator/OutputExtractors/IOutputValueExtractorVisitor.h>
#include <OpenSimCreator/OutputExtractors/IStringOutputValueExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/OutputExtractors/LambdaOutputValueExtractorVisitor.h>

#include <array>
#include <span>
#include <string>

using namespace osc;

OutputExtractorDataType osc::IOutputExtractor::getOutputType() const
{
    OutputExtractorDataType rv = OutputExtractorDataType::String;
    LambdaOutputValueExtractorVisitor visitor{
        [&rv](IFloatOutputValueExtractor const&)
        {
            rv = OutputExtractorDataType::Float;
        },
        [&rv](IStringOutputValueExtractor const&)
        {
            rv = OutputExtractorDataType::String;
        },
    };
    implAccept(visitor);
    return rv;
}

float osc::IOutputExtractor::getValueFloat(
    OpenSim::Component const& component,
    SimulationReport const& report) const
{
    std::span<SimulationReport const> reports(&report, 1);
    std::array<float, 1> out{};
    getValuesFloat(component, reports, out);
    return out.front();
}

void osc::IOutputExtractor::getValuesFloat(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports,
    std::span<float> overwriteOut) const
{
    LambdaOutputValueExtractorVisitor visitor{
        [&component, &reports, &overwriteOut](IFloatOutputValueExtractor const& extractor)
        {
            extractor.extractFloats(component, reports, overwriteOut);
        },
        [](IStringOutputValueExtractor const&)
        {},
    };
    implAccept(visitor);
}

std::string osc::IOutputExtractor::getValueString(
    OpenSim::Component const& component,
    SimulationReport const& report) const
{
    std::string rv;
    LambdaOutputValueExtractorVisitor visitor{
        [&component, &report, &rv](IFloatOutputValueExtractor const& extractor)
        {
            std::span<SimulationReport const> reports(&report, 1);
            std::array<float, 1> out{};
            extractor.extractFloats(component, reports, out);
            rv = std::to_string(out.front());
        },
        [&component, &report, &rv](IStringOutputValueExtractor const& extractor)
        {
            rv = extractor.extractString(component, report);
        },
    };
    implAccept(visitor);
    return rv;
}

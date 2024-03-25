#include "IOutputExtractor.h"

#include <OpenSimCreator/OutputExtractors/IFloatOutputValueExtractor.h>
#include <OpenSimCreator/OutputExtractors/IOutputValueExtractorVisitor.h>
#include <OpenSimCreator/OutputExtractors/IStringOutputValueExtractor.h>
#include <OpenSimCreator/OutputExtractors/IVec2OutputValueExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/OutputExtractors/LambdaOutputValueExtractorVisitor.h>

#include <oscar/Maths/Constants.h>

#include <array>
#include <span>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

OutputExtractorDataType osc::IOutputExtractor::getOutputType() const
{
    OutputExtractorDataType rv = OutputExtractorDataType::String;
    LambdaOutputValueExtractorVisitor visitor{
        [&rv](IFloatOutputValueExtractor const&)
        {
            rv = OutputExtractorDataType::Float;
        },
        [&rv](IVec2OutputValueExtractor const&)
        {
            rv = OutputExtractorDataType::Vec2;
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
    std::span<SimulationReport const> reports{&report, 1};
    float rv = quiet_nan_v<float>;
    getValuesFloat(component, reports, [&rv](float v) { rv = v; });
    return rv;
}

void osc::IOutputExtractor::getValuesFloat(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports,
    std::function<void(float)> const& consumer) const
{
    LambdaOutputValueExtractorVisitor visitor{
        [&component, &reports, &consumer](IFloatOutputValueExtractor const& extractor)
        {
            extractor.extractFloats(component, reports, consumer);
        },
        [](IVec2OutputValueExtractor const&) {},
        [](IStringOutputValueExtractor const&) {},
    };
    implAccept(visitor);
}

std::vector<float> osc::IOutputExtractor::slurpValuesFloat(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports) const
{
    std::vector<float> rv;
    rv.reserve(reports.size());
    getValuesFloat(component, reports, [&rv](float v) { rv.push_back(v); });
    return rv;
}

Vec2 osc::IOutputExtractor::getValueVec2(
    OpenSim::Component const& component,
    SimulationReport const& report) const
{
    std::span<SimulationReport const> reports(&report, 1);
    Vec2 rv{quiet_nan_v<float>};
    getValuesVec2(component, reports, [&rv](Vec2 v) { rv = v; });
    return rv;
}

void osc::IOutputExtractor::getValuesVec2(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports,
    std::function<void(Vec2)> const& consumer) const
{
    LambdaOutputValueExtractorVisitor visitor{
        [](IFloatOutputValueExtractor const&) {},
        [&component, &reports, &consumer](IVec2OutputValueExtractor const& extractor)
        {
            extractor.extractVec2s(component, reports, consumer);
        },
        [](IStringOutputValueExtractor const&) {},
    };
    implAccept(visitor);
}

std::vector<Vec2> osc::IOutputExtractor::slurpValuesVec2(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports) const
{
    std::vector<Vec2> rv;
    rv.reserve(reports.size());
    getValuesVec2(component, reports, [&rv](Vec2 v) { rv.push_back(v); });
    return rv;
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
            float fv = quiet_nan_v<float>;
            extractor.extractFloats(component, reports, [&fv](float v) { fv = v; });
            rv = std::to_string(fv);
        },
        [&component, &report, &rv](IVec2OutputValueExtractor const& extractor)
        {
            std::span<SimulationReport const> reports(&report, 1);
            Vec2 v2v{quiet_nan_v<float>};
            extractor.extractVec2s(component, reports, [&v2v](Vec2 v)
            {
                v2v = v;
            });
            std::stringstream ss;
            ss << v2v.x << ", " << v2v.y;
            rv = std::move(ss).str();
        },
        [&component, &report, &rv](IStringOutputValueExtractor const& extractor)
        {
            rv = extractor.extractString(component, report);
        },
    };
    implAccept(visitor);
    return rv;
}

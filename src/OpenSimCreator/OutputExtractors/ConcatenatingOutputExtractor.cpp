#include "ConcatenatingOutputExtractor.h"

#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/IOutputValueExtractorVisitor.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractorDataType.h>

#include <oscar/Maths/Constants.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/HashHelpers.h>

#include <cstddef>
#include <sstream>
#include <string>

using namespace osc;

namespace
{
    OutputExtractorDataType CalcOutputType(OutputExtractor const& a, OutputExtractor const& b)
    {
        static_assert(NumOptions<OutputExtractorDataType>() == 3);

        OutputExtractorDataType const aType = a.getOutputType();
        OutputExtractorDataType const bType = b.getOutputType();

        if (aType == OutputExtractorDataType::Float && bType == OutputExtractorDataType::Float) {
            return OutputExtractorDataType::Vec2;
        }
        else {
            return OutputExtractorDataType::String;
        }
    }

    std::string CalcLabel(OutputExtractorDataType concatenatedType, OutputExtractor const& a, OutputExtractor const& b)
    {
        static_assert(NumOptions<OutputExtractorDataType>() == 3);

        if (concatenatedType == OutputExtractorDataType::Vec2) {
            std::stringstream ss;
            ss << a.getName() << " vs. " << b.getName();
            return std::move(ss).str();
        }
        else {
            std::stringstream ss;
            ss << a.getName() << " + " << b.getName();
            return std::move(ss).str();
        }
    }
}

osc::ConcatenatingOutputExtractor::ConcatenatingOutputExtractor(
    OutputExtractor first_,
    OutputExtractor second_) :

    m_First{std::move(first_)},
    m_Second{std::move(second_)},
    m_OutputType{CalcOutputType(m_First, m_Second)},
    m_Label{CalcLabel(m_OutputType, m_First, m_Second)}
{}

void osc::ConcatenatingOutputExtractor::implAccept(IOutputValueExtractorVisitor& visitor) const
{
    static_assert(NumOptions<OutputExtractorDataType>() == 3);

    if (m_OutputType == OutputExtractorDataType::Vec2) {
        visitor(static_cast<IVec2OutputValueExtractor const&>(*this));
    }
    else {
        visitor(static_cast<IStringOutputValueExtractor const&>(*this));
    }
}

size_t osc::ConcatenatingOutputExtractor::implGetHash() const
{
    return HashOf(m_First, m_Second);
}

bool osc::ConcatenatingOutputExtractor::implEquals(IOutputExtractor const& other) const
{
    if (&other == this) {
        return true;
    }
    if (auto* ptr = dynamic_cast<ConcatenatingOutputExtractor const*>(&other)) {
        return ptr->m_First == m_First && ptr->m_Second == m_Second;
    }
    return false;
}

void osc::ConcatenatingOutputExtractor::implExtractFloats(
    OpenSim::Component const& component,
    std::span<SimulationReport const> reports,
    std::span<Vec2> overwriteOut) const
{
    if (m_OutputType != OutputExtractorDataType::Vec2) {
        return;  // invalid method call
    }

    OSC_ASSERT(reports.size() == overwriteOut.size());

    // TODO: these allocations are entirely because the IOutputExtractor API design
    //       currently requires a contiguous output, and there's no easy way to use
    //       the span without some godawful amount of shuffling floats around

    std::vector<float> firstOut(overwriteOut.size(), quiet_nan_v<float>);
    std::vector<float> secondOut(overwriteOut.size(), quiet_nan_v<float>);

    m_First.getValuesFloat(component, reports, firstOut);
    m_Second.getValuesFloat(component, reports, secondOut);

    for (size_t i = 0; i < overwriteOut.size(); ++i) {
        overwriteOut[i].x = firstOut[i];
        overwriteOut[i].y = secondOut[i];
    }
}

std::string osc::ConcatenatingOutputExtractor::implExtractString(
    OpenSim::Component const& component,
    SimulationReport const& report) const
{
    if (m_OutputType != OutputExtractorDataType::String) {
        return {};  // invalid method call
    }

    return m_First.getValueString(component, report) + m_Second.getValueString(component, report);
}

#include "ConcatenatingOutputExtractor.h"

#include <libopensimcreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractorDataType.h>

#include <liboscar/Maths/Constants.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/Conversion.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Utils/HashHelpers.h>

#include <cstddef>
#include <sstream>
#include <string>

using namespace osc;

namespace
{
    OutputExtractorDataType CalcOutputType(const OutputExtractor& a, const OutputExtractor& b)
    {
        static_assert(num_options<OutputExtractorDataType>() == 3);

        const OutputExtractorDataType aType = a.getOutputType();
        const OutputExtractorDataType bType = b.getOutputType();

        if (aType == OutputExtractorDataType::Float && bType == OutputExtractorDataType::Float) {
            return OutputExtractorDataType::Vector2;
        }
        else {
            return OutputExtractorDataType::String;
        }
    }

    std::string CalcLabel(OutputExtractorDataType concatenatedType, const OutputExtractor& a, const OutputExtractor& b)
    {
        static_assert(num_options<OutputExtractorDataType>() == 3);

        if (concatenatedType == OutputExtractorDataType::Vector2) {
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

OutputValueExtractor osc::ConcatenatingOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component& comp) const
{
    static_assert(num_options<OutputExtractorDataType>() == 3);

    if (m_OutputType == OutputExtractorDataType::Vector2) {
        auto extractor = [lhs = m_First.getOutputValueExtractor(comp), rhs = m_Second.getOutputValueExtractor(comp)](const SimulationReport& report)
        {
            const auto lv = to<float>(lhs(report));
            const auto rv = to<float>(rhs(report));

            return Variant{Vector2{lv, rv}};
        };
        return OutputValueExtractor{std::move(extractor)};
    }
    else {
        auto extractor = [lhs = m_First.getOutputValueExtractor(comp), rhs = m_Second.getOutputValueExtractor(comp)](const SimulationReport& report)
        {
            return Variant{to<std::string>(lhs(report)) + to<std::string>(rhs(report))};
        };
        return OutputValueExtractor{std::move(extractor)};
    }
}

size_t osc::ConcatenatingOutputExtractor::implGetHash() const
{
    return hash_of(m_First, m_Second);
}

bool osc::ConcatenatingOutputExtractor::implEquals(const IOutputExtractor& other) const
{
    if (&other == this) {
        return true;
    }
    if (auto* ptr = dynamic_cast<const ConcatenatingOutputExtractor*>(&other)) {
        return ptr->m_First == m_First && ptr->m_Second == m_Second;
    }
    return false;
}

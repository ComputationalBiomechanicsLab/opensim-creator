#include "ComponentOutputExtractor.h"

#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/OutputValueExtractor.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Common/ComponentPath.h>
#include <SimTKcommon/SmallMatrix.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/Perf.h>

#include <cmath>
#include <algorithm>
#include <array>
#include <concepts>
#include <memory>
#include <span>
#include <sstream>
#include <typeinfo>
#include <utility>

using namespace osc;

// other helpers
namespace
{
    std::string GenerateComponentOutputLabel(
        OpenSim::ComponentPath const& cp,
        std::string const& outputName,
        ComponentOutputSubfield subfield)
    {
        std::stringstream ss;
        ss << cp.toString() << '[' << outputName;
        if (auto label = GetOutputSubfieldLabel(subfield))
        {
            ss << '.' << *label;
        }
        ss << ']';
        return std::move(ss).str();
    }

    Variant NaNFloatingPointCallback(SimulationReport const&)
    {
        return Variant{quiet_nan_v<float>};
    }

    Variant BlankStringCallback(SimulationReport const&)
    {
        return Variant{std::string{}};
    }

    using NullCallbackFnPointer = Variant(*)(SimulationReport const&);
}

class osc::ComponentOutputExtractor::Impl final {
public:
    Impl(OpenSim::AbstractOutput const& ao,
         ComponentOutputSubfield subfield) :

        m_ComponentAbsPath{GetAbsolutePath(GetOwnerOrThrow(ao))},
        m_OutputName{ao.getName()},
        m_Label{GenerateComponentOutputLabel(m_ComponentAbsPath, m_OutputName, subfield)},
        m_OutputTypeid{&typeid(ao)},
        m_ExtractorFunc{GetExtractorFuncOrNull(ao, subfield)}
    {}

    std::unique_ptr<Impl> clone() const { return std::make_unique<Impl>(*this); }

    OpenSim::ComponentPath const& getComponentAbsPath() const { return m_ComponentAbsPath; }

    CStringView getName() const { return m_Label; }
    CStringView getDescription() const { return CStringView{}; }

    OutputExtractorDataType getOutputType() const
    {
        return m_ExtractorFunc ? OutputExtractorDataType::Float : OutputExtractorDataType::String;
    }

    OutputValueExtractor getOutputValueExtractor(OpenSim::Component const& component) const
    {
        OutputExtractorDataType const datatype = getOutputType();
        NullCallbackFnPointer const nullCallback = datatype == OutputExtractorDataType::Float ? NaNFloatingPointCallback : BlankStringCallback;

        OpenSim::AbstractOutput const* const ao = FindOutput(component, m_ComponentAbsPath, m_OutputName);

        if (not ao) {
            return OutputValueExtractor{nullCallback};  // cannot find output
        }
        if (typeid(*ao) != *m_OutputTypeid) {
            return OutputValueExtractor{nullCallback};  // output has changed
        }

        if (datatype == OutputExtractorDataType::Float) {
            return OutputValueExtractor{[func = m_ExtractorFunc, ao](SimulationReport const& report)
            {
                return Variant{static_cast<float>(func(*ao, report.getState()))};
            }};
        }
        else {
            return OutputValueExtractor{[ao](SimulationReport const& report)
            {
                return Variant{ao->getValueAsString(report.getState())};
            }};
        }
    }

    size_t getHash() const
    {
        return HashOf(m_ComponentAbsPath.toString(), m_OutputName, m_Label, m_OutputTypeid, m_ExtractorFunc);
    }

    bool equals(IOutputExtractor const& other)
    {
        auto const* const otherT = dynamic_cast<ComponentOutputExtractor const*>(&other);
        if (!otherT) {
            return false;
        }

        ComponentOutputExtractor::Impl const* const otherImpl = otherT->m_Impl.get();
        if (otherImpl == this) {
            return true;
        }

        return
            m_ComponentAbsPath == otherImpl->m_ComponentAbsPath &&
            m_OutputName == otherImpl->m_OutputName &&
            m_Label == otherImpl->m_Label &&
            m_OutputTypeid == otherImpl->m_OutputTypeid &&
            m_ExtractorFunc == otherImpl->m_ExtractorFunc;
    }

private:
    OpenSim::ComponentPath m_ComponentAbsPath;
    std::string m_OutputName;
    std::string m_Label;
    std::type_info const* m_OutputTypeid;
    SubfieldExtractorFunc m_ExtractorFunc;
};


// public API

osc::ComponentOutputExtractor::ComponentOutputExtractor(OpenSim::AbstractOutput const& ao,
                                                        ComponentOutputSubfield subfield) :
    m_Impl{std::make_unique<Impl>(ao, subfield)}
{
}
osc::ComponentOutputExtractor::ComponentOutputExtractor(ComponentOutputExtractor const&) = default;
osc::ComponentOutputExtractor::ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept = default;
osc::ComponentOutputExtractor& osc::ComponentOutputExtractor::operator=(ComponentOutputExtractor const&) = default;
osc::ComponentOutputExtractor& osc::ComponentOutputExtractor::operator=(ComponentOutputExtractor&&) noexcept = default;
osc::ComponentOutputExtractor::~ComponentOutputExtractor() noexcept = default;

OpenSim::ComponentPath const& osc::ComponentOutputExtractor::getComponentAbsPath() const
{
    return m_Impl->getComponentAbsPath();
}

CStringView osc::ComponentOutputExtractor::implGetName() const
{
    return m_Impl->getName();
}

CStringView osc::ComponentOutputExtractor::implGetDescription() const
{
    return m_Impl->getDescription();
}

OutputExtractorDataType osc::ComponentOutputExtractor::implGetOutputType() const
{
    return m_Impl->getOutputType();
}

OutputValueExtractor osc::ComponentOutputExtractor::implGetOutputValueExtractor(OpenSim::Component const& component) const
{
    return m_Impl->getOutputValueExtractor(component);
}

std::size_t osc::ComponentOutputExtractor::implGetHash() const
{
    return m_Impl->getHash();
}

bool osc::ComponentOutputExtractor::implEquals(IOutputExtractor const& other) const
{
    return m_Impl->equals(other);
}

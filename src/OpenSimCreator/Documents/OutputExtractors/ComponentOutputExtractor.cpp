#include "ComponentOutputExtractor.h"

#include <OpenSimCreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>
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
        const OpenSim::ComponentPath& cp,
        const std::string& outputName,
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

    Variant NaNFloatingPointCallback(const SimulationReport&)
    {
        return Variant{quiet_nan_v<float>};
    }

    Variant BlankStringCallback(const SimulationReport&)
    {
        return Variant{std::string{}};
    }

    using NullCallbackFnPointer = Variant(*)(const SimulationReport&);
}

class osc::ComponentOutputExtractor::Impl final {
public:
    Impl(const OpenSim::AbstractOutput& ao,
         ComponentOutputSubfield subfield) :

        m_ComponentAbsPath{GetAbsolutePath(GetOwnerOrThrow(ao))},
        m_OutputName{ao.getName()},
        m_Label{GenerateComponentOutputLabel(m_ComponentAbsPath, m_OutputName, subfield)},
        m_OutputTypeid{&typeid(ao)},
        m_ExtractorFunc{GetExtractorFuncOrNull(ao, subfield)}
    {}

    std::unique_ptr<Impl> clone() const { return std::make_unique<Impl>(*this); }

    const OpenSim::ComponentPath& getComponentAbsPath() const { return m_ComponentAbsPath; }

    CStringView getName() const { return m_Label; }
    CStringView getDescription() const { return CStringView{}; }

    OutputExtractorDataType getOutputType() const
    {
        return m_ExtractorFunc ? OutputExtractorDataType::Float : OutputExtractorDataType::String;
    }

    OutputValueExtractor getOutputValueExtractor(const OpenSim::Component& component) const
    {
        const OutputExtractorDataType datatype = getOutputType();
        const NullCallbackFnPointer nullCallback = datatype == OutputExtractorDataType::Float ? NaNFloatingPointCallback : BlankStringCallback;

        const OpenSim::AbstractOutput* const ao = FindOutput(component, m_ComponentAbsPath, m_OutputName);

        if (not ao) {
            return OutputValueExtractor{nullCallback};  // cannot find output
        }
        if (typeid(*ao) != *m_OutputTypeid) {
            return OutputValueExtractor{nullCallback};  // output has changed
        }

        if (datatype == OutputExtractorDataType::Float) {
            return OutputValueExtractor{[func = m_ExtractorFunc, ao](const SimulationReport& report)
            {
                return Variant{static_cast<float>(func(*ao, report.getState()))};
            }};
        }
        else {
            return OutputValueExtractor{[ao](const SimulationReport& report)
            {
                return Variant{ao->getValueAsString(report.getState())};
            }};
        }
    }

    size_t getHash() const
    {
        return hash_of(m_ComponentAbsPath.toString(), m_OutputName, m_Label, m_OutputTypeid, m_ExtractorFunc);
    }

    bool equals(const IOutputExtractor& other)
    {
        const auto* const otherT = dynamic_cast<const ComponentOutputExtractor*>(&other);
        if (!otherT) {
            return false;
        }

        const ComponentOutputExtractor::Impl* const otherImpl = otherT->m_Impl.get();
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
    const std::type_info* m_OutputTypeid;
    SubfieldExtractorFunc m_ExtractorFunc;
};


// public API

osc::ComponentOutputExtractor::ComponentOutputExtractor(
    const OpenSim::AbstractOutput& ao,
    ComponentOutputSubfield subfield) :

    m_Impl{std::make_unique<Impl>(ao, subfield)}
{}
osc::ComponentOutputExtractor::ComponentOutputExtractor(const ComponentOutputExtractor&) = default;
osc::ComponentOutputExtractor::ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept = default;
osc::ComponentOutputExtractor& osc::ComponentOutputExtractor::operator=(const ComponentOutputExtractor&) = default;
osc::ComponentOutputExtractor& osc::ComponentOutputExtractor::operator=(ComponentOutputExtractor&&) noexcept = default;
osc::ComponentOutputExtractor::~ComponentOutputExtractor() noexcept = default;

const OpenSim::ComponentPath& osc::ComponentOutputExtractor::getComponentAbsPath() const
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

OutputValueExtractor osc::ComponentOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component& component) const
{
    return m_Impl->getOutputValueExtractor(component);
}

std::size_t osc::ComponentOutputExtractor::implGetHash() const
{
    return m_Impl->getHash();
}

bool osc::ComponentOutputExtractor::implEquals(const IOutputExtractor& other) const
{
    return m_Impl->equals(other);
}

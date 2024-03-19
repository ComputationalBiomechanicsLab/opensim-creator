#include "ComponentOutputExtractor.h"

#include <OpenSimCreator/OutputExtractors/IFloatOutputValueExtractor.h>
#include <OpenSimCreator/OutputExtractors/IStringOutputValueExtractor.h>
#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/IOutputValueExtractorVisitor.h>
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
}

class osc::ComponentOutputExtractor::Impl final :
    private IFloatOutputValueExtractor,
    private IStringOutputValueExtractor {
public:
    Impl(OpenSim::AbstractOutput const& ao,
         ComponentOutputSubfield subfield) :

        m_ComponentAbsPath{GetAbsolutePath(GetOwnerOrThrow(ao))},
        m_OutputName{ao.getName()},
        m_Label{GenerateComponentOutputLabel(m_ComponentAbsPath, m_OutputName, subfield)},
        m_OutputType{&typeid(ao)},
        m_ExtractorFunc{GetExtractorFuncOrNull(ao, subfield)}
    {}

    std::unique_ptr<Impl> clone() const { return std::make_unique<Impl>(*this); }

    OpenSim::ComponentPath const& getComponentAbsPath() const { return m_ComponentAbsPath; }

    CStringView getName() const { return m_Label; }
    CStringView getDescription() const { return CStringView{}; }

    void accept(IOutputValueExtractorVisitor& visitor)
    {
        if (m_ExtractorFunc) {
            visitor(static_cast<IFloatOutputValueExtractor const&>(*this));
        }
        else {
            visitor(static_cast<IStringOutputValueExtractor const&>(*this));
        }
    }

    size_t getHash() const
    {
        return HashOf(m_ComponentAbsPath.toString(), m_OutputName, m_Label, m_OutputType, m_ExtractorFunc);
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
            m_OutputType == otherImpl->m_OutputType &&
            m_ExtractorFunc == otherImpl->m_ExtractorFunc;
    }

    void implExtractFloats(
        OpenSim::Component const& c,
        std::span<SimulationReport const> reports,
        std::span<float> out) const override
    {
        OSC_PERF("ComponentOutputExtractor::getValuesFloat");
        OSC_ASSERT_ALWAYS(reports.size() == out.size());

        OpenSim::AbstractOutput const* const ao = FindOutput(c, m_ComponentAbsPath, m_OutputName);

        if (!ao || typeid(*ao) != *m_OutputType || !m_ExtractorFunc)
        {
            // cannot find output
            // or the type of the output changed
            // or don't know how to extract a value from the output
            fill(out, quiet_nan_v<float>);
            return;
        }

        OSC_PERF("ComponentOutputExtractor::getValuesFloat");

        for (size_t i = 0; i < reports.size(); ++i)
        {
            out[i] = static_cast<float>(m_ExtractorFunc(*ao, reports[i].getState()));
        }
    }

    std::string implExtractString(
        OpenSim::Component const& component,
        SimulationReport const& report) const override
    {
        OpenSim::AbstractOutput const* const ao = FindOutput(component, m_ComponentAbsPath, m_OutputName);
        if (!ao) {
            return std::string{};
        }

        if (m_ExtractorFunc) {
            return std::to_string(m_ExtractorFunc(*ao, report.getState()));
        }
        else {
            return ao->getValueAsString(report.getState());
        }
    }

private:
    OpenSim::ComponentPath m_ComponentAbsPath;
    std::string m_OutputName;
    std::string m_Label;
    std::type_info const* m_OutputType;
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

void osc::ComponentOutputExtractor::implAccept(IOutputValueExtractorVisitor& visitor) const
{
    m_Impl->accept(visitor);
}

std::size_t osc::ComponentOutputExtractor::implGetHash() const
{
    return m_Impl->getHash();
}

bool osc::ComponentOutputExtractor::implEquals(IOutputExtractor const& other) const
{
    return m_Impl->equals(other);
}

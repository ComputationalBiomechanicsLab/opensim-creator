#include "component_output_extractor.h"

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>
#include <libopynsim/documents/state_view_with_metadata.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/maths/constants.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/hash_helpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Common/ComponentPath.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <typeinfo>
#include <utility>

using namespace opyn;

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

    OutputValueExtractor MakeNullExtractor(OutputExtractorDataType type)
    {
        static_assert(osc::num_options<OutputExtractorDataType>() == 3);
        switch (type) {
        case OutputExtractorDataType::Float:   return OutputValueExtractor::constant(osc::quiet_nan_v<float>);
        case OutputExtractorDataType::Vector2: return OutputValueExtractor::constant(osc::Vector2{osc::quiet_nan_v<float>});
        default:                               return OutputValueExtractor::constant(std::string{});
        }
    }
}

class opyn::ComponentOutputExtractor::Impl final {
public:
    Impl(const OpenSim::AbstractOutput& ao,
         ComponentOutputSubfield subfield) :

        m_ComponentAbsPath{GetAbsolutePath(GetOwnerOrThrow(ao))},
        m_OutputName{ao.getName()},
        m_Label{GenerateComponentOutputLabel(m_ComponentAbsPath, m_OutputName, subfield)},
        m_OutputTypeid{&typeid(ao)},
        m_ExtractorFunc{GetExtractorFuncOrNull(ao, subfield)}
    {}

    friend bool operator==(const Impl&, const Impl&) = default;

    std::unique_ptr<Impl> clone() const { return std::make_unique<Impl>(*this); }

    const OpenSim::ComponentPath& getComponentAbsPath() const { return m_ComponentAbsPath; }

    osc::CStringView getName() const { return m_Label; }
    osc::CStringView getDescription() const { return {}; }

    OutputExtractorDataType getOutputType() const
    {
        return m_ExtractorFunc ? OutputExtractorDataType::Float : OutputExtractorDataType::String;
    }

    OutputValueExtractor getOutputValueExtractor(const OpenSim::Component& component) const
    {
        const OutputExtractorDataType datatype = getOutputType();
        const OpenSim::AbstractOutput* const ao = FindOutput(component, m_ComponentAbsPath, m_OutputName);

        if (not ao) {
            return MakeNullExtractor(datatype);  // cannot find output
        }
        if (typeid(*ao) != *m_OutputTypeid) {
            return MakeNullExtractor(datatype);  // output has changed
        }

        if (datatype == OutputExtractorDataType::Float) {
            return OutputValueExtractor{[func = m_ExtractorFunc, ao](const StateViewWithMetadata& state)
            {
                return osc::Variant{static_cast<float>(func(*ao, state.getState()))};
            }};
        }
        else {
            return OutputValueExtractor{[ao](const StateViewWithMetadata& state)
            {
                return osc::Variant{ao->getValueAsString(state.getState())};
            }};
        }
    }

    size_t getHash() const
    {
        return osc::hash_of(m_ComponentAbsPath.toString(), m_OutputName, m_Label, m_OutputTypeid, m_ExtractorFunc);
    }

    bool equals(const OutputExtractor& other)
    {
        const auto* const otherT = dynamic_cast<const ComponentOutputExtractor*>(&other);
        if (!otherT) {
            return false;
        }

        const ComponentOutputExtractor::Impl* const otherImpl = otherT->m_Impl.get();
        if (otherImpl == this) {
            return true;
        }

        return *otherImpl == *this;
    }

private:
    OpenSim::ComponentPath m_ComponentAbsPath;
    std::string m_OutputName;
    std::string m_Label;
    const std::type_info* m_OutputTypeid;
    SubfieldExtractorFunc m_ExtractorFunc;
};

opyn::ComponentOutputExtractor::ComponentOutputExtractor(
    const OpenSim::AbstractOutput& ao,
    ComponentOutputSubfield subfield) :

    m_Impl{std::make_unique<Impl>(ao, subfield)}
{}
opyn::ComponentOutputExtractor::ComponentOutputExtractor(const ComponentOutputExtractor&) = default;
opyn::ComponentOutputExtractor::ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept = default;
ComponentOutputExtractor& opyn::ComponentOutputExtractor::operator=(const ComponentOutputExtractor&) = default;
ComponentOutputExtractor& opyn::ComponentOutputExtractor::operator=(ComponentOutputExtractor&&) noexcept = default;
opyn::ComponentOutputExtractor::~ComponentOutputExtractor() noexcept = default;

const OpenSim::ComponentPath& opyn::ComponentOutputExtractor::getComponentAbsPath() const
{
    return m_Impl->getComponentAbsPath();
}

osc::CStringView opyn::ComponentOutputExtractor::implGetName() const
{
    return m_Impl->getName();
}

osc::CStringView opyn::ComponentOutputExtractor::implGetDescription() const
{
    return m_Impl->getDescription();
}

OutputExtractorDataType opyn::ComponentOutputExtractor::implGetOutputType() const
{
    return m_Impl->getOutputType();
}

OutputValueExtractor opyn::ComponentOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component& component) const
{
    return m_Impl->getOutputValueExtractor(component);
}

std::size_t opyn::ComponentOutputExtractor::implGetHash() const
{
    return m_Impl->getHash();
}

bool opyn::ComponentOutputExtractor::implEquals(const OutputExtractor& other) const
{
    return m_Impl->equals(other);
}

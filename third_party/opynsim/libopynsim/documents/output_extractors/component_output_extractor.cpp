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
        osc::ComponentOutputSubfield subfield)
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

    osc::OutputValueExtractor MakeNullExtractor(osc::OutputExtractorDataType type)
    {
        static_assert(osc::num_options<osc::OutputExtractorDataType>() == 3);
        switch (type) {
        case osc::OutputExtractorDataType::Float:   return osc::OutputValueExtractor::constant(osc::quiet_nan_v<float>);
        case osc::OutputExtractorDataType::Vector2: return osc::OutputValueExtractor::constant(osc::Vector2{osc::quiet_nan_v<float>});
        default:                                    return osc::OutputValueExtractor::constant(std::string{});
        }
    }
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

    friend bool operator==(const Impl&, const Impl&) = default;

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
                return Variant{static_cast<float>(func(*ao, state.getState()))};
            }};
        }
        else {
            return OutputValueExtractor{[ao](const StateViewWithMetadata& state)
            {
                return Variant{ao->getValueAsString(state.getState())};
            }};
        }
    }

    size_t getHash() const
    {
        return hash_of(m_ComponentAbsPath.toString(), m_OutputName, m_Label, m_OutputTypeid, m_ExtractorFunc);
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

osc::CStringView osc::ComponentOutputExtractor::implGetName() const
{
    return m_Impl->getName();
}

osc::CStringView osc::ComponentOutputExtractor::implGetDescription() const
{
    return m_Impl->getDescription();
}

osc::OutputExtractorDataType osc::ComponentOutputExtractor::implGetOutputType() const
{
    return m_Impl->getOutputType();
}

osc::OutputValueExtractor osc::ComponentOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component& component) const
{
    return m_Impl->getOutputValueExtractor(component);
}

std::size_t osc::ComponentOutputExtractor::implGetHash() const
{
    return m_Impl->getHash();
}

bool osc::ComponentOutputExtractor::implEquals(const OutputExtractor& other) const
{
    return m_Impl->equals(other);
}

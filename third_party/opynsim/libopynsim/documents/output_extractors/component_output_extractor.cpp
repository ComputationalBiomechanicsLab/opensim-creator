#include "component_output_extractor.h"

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>
#include <libopynsim/documents/state_view_with_metadata.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/maths/constants.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/hash_helpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Common/ComponentPath.h>

#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <typeinfo>

using namespace opyn;

// other helpers
namespace
{
    std::string generate_component_output_label(
        const OpenSim::ComponentPath& cp,
        const std::string& output_name,
        ComponentOutputSubfield subfield)
    {
        const auto label = get_output_subfield_label(subfield);
        return std::format("{}[{}{}]",
            cp.toString(),
            output_name,
            label ? std::format(".{}", *label) : ""
        );
    }

    OutputValueExtractor make_null_extractor(OutputExtractorDataType type)
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

        component_abs_path_{GetAbsolutePath(GetOwnerOrThrow(ao))},
        output_name_{ao.getName()},
        label_{generate_component_output_label(component_abs_path_, output_name_, subfield)},
        output_type_id_{&typeid(ao)},
        extractor_func_{get_extractor_func_or_null(ao, subfield)}
    {}

    friend bool operator==(const Impl&, const Impl&) = default;

    std::unique_ptr<Impl> clone() const { return std::make_unique<Impl>(*this); }

    const OpenSim::ComponentPath& component_abs_path() const { return component_abs_path_; }

    osc::CStringView name() const { return label_; }
    osc::CStringView description() const { return {}; }

    OutputExtractorDataType output_type() const
    {
        return extractor_func_ ? OutputExtractorDataType::Float : OutputExtractorDataType::String;
    }

    OutputValueExtractor output_value_extractor(const OpenSim::Component& component) const
    {
        const OutputExtractorDataType datatype = output_type();
        const OpenSim::AbstractOutput* const ao = FindOutput(component, component_abs_path_, output_name_);

        if (not ao) {
            return make_null_extractor(datatype);  // cannot find output
        }
        if (typeid(*ao) != *output_type_id_) {
            return make_null_extractor(datatype);  // output has changed
        }

        if (datatype == OutputExtractorDataType::Float) {
            return OutputValueExtractor{[func = extractor_func_, ao](const StateViewWithMetadata& state)
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

    size_t hash() const
    {
        return osc::hash_of(component_abs_path_.toString(), output_name_, label_, output_type_id_, extractor_func_);
    }

    bool equals(const OutputExtractor& other)
    {
        const auto* const other_t = dynamic_cast<const ComponentOutputExtractor*>(&other);
        if (not other_t) {
            return false;
        }

        const ComponentOutputExtractor::Impl* const other_impl = other_t->impl_.get();
        if (other_impl == this) {
            return true;
        }

        return *other_impl == *this;
    }

private:
    OpenSim::ComponentPath component_abs_path_;
    std::string output_name_;
    std::string label_;
    const std::type_info* output_type_id_;
    SubfieldExtractorFunc extractor_func_;
};

opyn::ComponentOutputExtractor::ComponentOutputExtractor(
    const OpenSim::AbstractOutput& ao,
    ComponentOutputSubfield subfield) :

    impl_{std::make_unique<Impl>(ao, subfield)}
{}
opyn::ComponentOutputExtractor::ComponentOutputExtractor(const ComponentOutputExtractor&) = default;
opyn::ComponentOutputExtractor::ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept = default;
ComponentOutputExtractor& opyn::ComponentOutputExtractor::operator=(const ComponentOutputExtractor&) = default;
ComponentOutputExtractor& opyn::ComponentOutputExtractor::operator=(ComponentOutputExtractor&&) noexcept = default;
opyn::ComponentOutputExtractor::~ComponentOutputExtractor() noexcept = default;

const OpenSim::ComponentPath& opyn::ComponentOutputExtractor::component_abs_path() const
{
    return impl_->component_abs_path();
}

osc::CStringView opyn::ComponentOutputExtractor::impl_name() const
{
    return impl_->name();
}

osc::CStringView opyn::ComponentOutputExtractor::impl_description() const
{
    return impl_->description();
}

OutputExtractorDataType opyn::ComponentOutputExtractor::impl_output_type() const
{
    return impl_->output_type();
}

OutputValueExtractor opyn::ComponentOutputExtractor::impl_output_value_extractor(const OpenSim::Component& component) const
{
    return impl_->output_value_extractor(component);
}

std::size_t opyn::ComponentOutputExtractor::impl_hash() const
{
    return impl_->hash();
}

bool opyn::ComponentOutputExtractor::impl_equals(const OutputExtractor& other) const
{
    return impl_->equals(other);
}

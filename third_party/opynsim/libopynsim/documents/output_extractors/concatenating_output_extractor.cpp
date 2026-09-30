#include "concatenating_output_extractor.h"

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>

#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/hash_helpers.h>

#include <cstddef>
#include <format>
#include <string>
#include <utility>

using namespace opyn;

namespace
{
    OutputExtractorDataType calc_output_type(const SharedOutputExtractor& a, const SharedOutputExtractor& b)
    {
        static_assert(osc::num_options<OutputExtractorDataType>() == 3);

        const OutputExtractorDataType a_type = a.output_type();
        const OutputExtractorDataType b_type = b.output_type();

        if (a_type == OutputExtractorDataType::Float && b_type == OutputExtractorDataType::Float) {
            return OutputExtractorDataType::Vector2;
        }
        else {
            return OutputExtractorDataType::String;
        }
    }

    std::string calc_label(
        OutputExtractorDataType concatenated_type,
        const SharedOutputExtractor& a,
        const SharedOutputExtractor& b)
    {
        static_assert(osc::num_options<OutputExtractorDataType>() == 3);

        if (concatenated_type == OutputExtractorDataType::Vector2) {
            return std::format("{} vs. {}", a.name(), b.name());
        }
        else {
            return std::format("{} + {}", a.name(), b.name());
        }
    }
}

opyn::ConcatenatingOutputExtractor::ConcatenatingOutputExtractor(
    SharedOutputExtractor first,
    SharedOutputExtractor second) :

    first_{std::move(first)},
    second_{std::move(second)},
    output_type_{calc_output_type(first_, second_)},
    label_{calc_label(output_type_, first_, second_)}
{}

OutputValueExtractor opyn::ConcatenatingOutputExtractor::impl_output_value_extractor(const OpenSim::Component& comp) const
{
    static_assert(osc::num_options<OutputExtractorDataType>() == 3);

    if (output_type_ == OutputExtractorDataType::Vector2) {
        auto extractor = [lhs = first_.output_value_extractor(comp), rhs = second_.output_value_extractor(comp)](const StateViewWithMetadata& state)
        {
            const auto lv = to<float>(lhs(state));
            const auto rv = to<float>(rhs(state));

            return osc::Variant{osc::Vector2{lv, rv}};
        };
        return OutputValueExtractor{std::move(extractor)};
    }
    else {
        auto extractor = [lhs = first_.output_value_extractor(comp), rhs = second_.output_value_extractor(comp)](const StateViewWithMetadata& state)
        {
            return osc::Variant{to<std::string>(lhs(state)) + to<std::string>(rhs(state))};
        };
        return OutputValueExtractor{std::move(extractor)};
    }
}

size_t opyn::ConcatenatingOutputExtractor::impl_hash() const
{
    return osc::hash_of(first_, second_);
}

bool opyn::ConcatenatingOutputExtractor::impl_equals(const OutputExtractor& other) const
{
    if (&other == this) {
        return true;
    }
    if (const auto* ptr = dynamic_cast<const ConcatenatingOutputExtractor*>(&other)) {
        return ptr->first_ == first_ && ptr->second_ == second_;
    }
    return false;
}

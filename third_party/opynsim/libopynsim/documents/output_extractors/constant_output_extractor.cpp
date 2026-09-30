#include "constant_output_extractor.h"

#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/hash_helpers.h>

#include <cstddef>

opyn::OutputValueExtractor opyn::ConstantOutputExtractor::impl_output_value_extractor(const OpenSim::Component&) const
{
    return opyn::OutputValueExtractor{[value = this->value_](const opyn::StateViewWithMetadata&)
    {
        return value;
    }};
}

size_t opyn::ConstantOutputExtractor::impl_hash() const
{
    return hash_of(name_, value_);
}

bool opyn::ConstantOutputExtractor::impl_equals(const OutputExtractor& other) const
{
    return osc::is_eq_downcasted<ConstantOutputExtractor>(*this, other);
}

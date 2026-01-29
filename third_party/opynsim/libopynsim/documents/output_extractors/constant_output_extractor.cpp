#include "constant_output_extractor.h"

#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/hash_helpers.h>

#include <cstddef>

opyn::OutputValueExtractor opyn::ConstantOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component&) const
{
    return opyn::OutputValueExtractor{[value = this->m_Value](const opyn::StateViewWithMetadata&)
    {
        return value;
    }};
}

size_t opyn::ConstantOutputExtractor::implGetHash() const
{
    return hash_of(m_Name, m_Value);
}

bool opyn::ConstantOutputExtractor::implEquals(const OutputExtractor& other) const
{
    return osc::is_eq_downcasted<ConstantOutputExtractor>(*this, other);
}

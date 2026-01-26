#include "constant_output_extractor.h"

#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/hash_helpers.h>

#include <cstddef>

using namespace osc;

OutputValueExtractor osc::ConstantOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component&) const
{
    return OutputValueExtractor{[value = this->m_Value](const StateViewWithMetadata&)
    {
        return value;
    }};
}

size_t osc::ConstantOutputExtractor::implGetHash() const
{
    return hash_of(m_Name, m_Value);
}

bool osc::ConstantOutputExtractor::implEquals(const OutputExtractor& other) const
{
    return is_eq_downcasted<ConstantOutputExtractor>(*this, other);
}

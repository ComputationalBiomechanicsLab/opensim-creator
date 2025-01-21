#include "ConstantOutputExtractor.h"

#include <libopensimcreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/HashHelpers.h>

#include <cstddef>

using namespace osc;

OutputValueExtractor osc::ConstantOutputExtractor::implGetOutputValueExtractor(const OpenSim::Component&) const
{
    return OutputValueExtractor{[value = this->m_Value](const SimulationReport&)
    {
        return value;
    }};
}

size_t osc::ConstantOutputExtractor::implGetHash() const
{
    return hash_of(m_Name, m_Value);
}

bool osc::ConstantOutputExtractor::implEquals(const IOutputExtractor& other) const
{
    return is_eq_downcasted<ConstantOutputExtractor>(*this, other);
}

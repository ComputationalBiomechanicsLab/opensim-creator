#include "ConstantOutputExtractor.h"

#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/HashHelpers.h>

#include <cstddef>

using namespace osc;

OutputValueExtractor osc::ConstantOutputExtractor::implGetOutputValueExtractor(OpenSim::Component const&) const
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

bool osc::ConstantOutputExtractor::implEquals(IOutputExtractor const& other) const
{
    return is_eq_downcasted<ConstantOutputExtractor>(*this, other);
}

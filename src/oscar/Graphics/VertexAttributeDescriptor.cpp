#include "VertexAttributeDescriptor.hpp"

#include <oscar/Graphics/Detail/VertexAttributeFormatDetails.hpp>

#include <cstddef>


size_t osc::VertexAttributeDescriptor::stride() const
{
    return detail::GetDetails(m_Format).stride();
}

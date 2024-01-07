#include "VertexAttributeDescriptor.hpp"

#include <oscar/Graphics/Detail/VertexAttributeFormatHelpers.hpp>

#include <cstddef>

size_t osc::VertexAttributeDescriptor::stride() const
{
    return detail::StrideOf(m_Format);
}

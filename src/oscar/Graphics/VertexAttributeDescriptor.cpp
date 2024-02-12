#include "VertexAttributeDescriptor.h"

#include <oscar/Graphics/Detail/VertexAttributeFormatHelpers.h>

#include <cstddef>

size_t osc::VertexAttributeDescriptor::stride() const
{
    return detail::StrideOf(m_Format);
}

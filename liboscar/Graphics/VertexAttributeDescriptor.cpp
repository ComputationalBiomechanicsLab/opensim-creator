#include "VertexAttributeDescriptor.h"

#include <liboscar/Graphics/Detail/VertexAttributeFormatHelpers.h>

#include <cstddef>

size_t osc::VertexAttributeDescriptor::stride() const
{
    return detail::stride_of(format_);
}

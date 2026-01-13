#include "VertexAttributeDescriptor.h"

#include <liboscar/graphics/detail/VertexAttributeFormatHelpers.h>

#include <cstddef>

size_t osc::VertexAttributeDescriptor::stride() const
{
    return detail::stride_of(format_);
}

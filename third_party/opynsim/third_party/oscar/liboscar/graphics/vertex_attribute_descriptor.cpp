#include "vertex_attribute_descriptor.h"

#include <liboscar/graphics/detail/vertex_attribute_format_helpers.h>

#include <cstddef>

size_t osc::VertexAttributeDescriptor::stride() const
{
    return detail::stride_of(format_);
}

#include "vertex_attribute_format_helpers.h"

#include <liboscar/graphics/vertex_attribute_format.h>

#include <gtest/gtest.h>

using namespace osc;
using namespace osc::detail;

TEST(stride_of, works_as_expected)
{
    static_assert(stride_of(VertexAttributeFormat::Float32x2) == 2*sizeof(float));
}

TEST(num_components_in, works_as_expected)
{
    static_assert(num_components_in(VertexAttributeFormat::Float32x3) == 3);
}

TEST(component_size, works_as_expected)
{
    static_assert(component_size(VertexAttributeFormat::Float32x3) == sizeof(float));
}

#include "vertex_attribute_helpers.h"

#include <liboscar/graphics/vertex_attribute.h>
#include <liboscar/graphics/vertex_attribute_format.h>

#include <gtest/gtest.h>

using namespace osc;
using namespace osc::detail;

TEST(default_format, works_as_expected)
{
    static_assert(default_format(VertexAttribute::Position) == VertexAttributeFormat::Float32x3);
}

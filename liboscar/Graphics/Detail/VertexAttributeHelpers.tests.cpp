#include "VertexAttributeHelpers.h"

#include <liboscar/Graphics/VertexAttribute.h>
#include <liboscar/Graphics/VertexAttributeFormat.h>

#include <gtest/gtest.h>

using namespace osc;
using namespace osc::detail;

TEST(default_format, works_as_expected)
{
    static_assert(default_format(VertexAttribute::Position) == VertexAttributeFormat::Float32x3);
}

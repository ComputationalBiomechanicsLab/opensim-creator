#include <oscar/Graphics/Detail/VertexAttributeHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

using namespace osc;
using namespace osc::detail;

TEST(default_format, WorksAsExpected)
{
    static_assert(default_format(VertexAttribute::Position) == VertexAttributeFormat::Float32x3);
}

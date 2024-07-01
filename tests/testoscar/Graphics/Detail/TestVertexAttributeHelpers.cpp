#include <oscar/Graphics/Detail/VertexAttributeHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

using namespace osc;
using namespace osc::detail;

TEST(default_format, works_as_expected)
{
    static_assert(default_format(VertexAttribute::Position) == VertexAttributeFormat::Float32x3);
}

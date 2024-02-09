#include <oscar/Graphics/Detail/VertexAttributeHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

using osc::detail::DefaultFormat;
using osc::VertexAttribute;
using osc::VertexAttributeFormat;

TEST(VertexAttributeHelpers, DefaultFormatWorks)
{
    static_assert(DefaultFormat(VertexAttribute::Position) == VertexAttributeFormat::Float32x3);
}

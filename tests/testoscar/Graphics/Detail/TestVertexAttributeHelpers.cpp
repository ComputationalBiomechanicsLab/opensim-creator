#include <oscar/Graphics/Detail/VertexAttributeHelpers.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>

using osc::detail::DefaultFormat;
using osc::VertexAttribute;
using osc::VertexAttributeFormat;

TEST(VertexAttributeHelpers, DefaultFormatWorks)
{
    static_assert(DefaultFormat(VertexAttribute::Position) == VertexAttributeFormat::Float32x3);
}

#include <oscar/Graphics/Detail/VertexAttributeFormatHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

using osc::detail::SizeOfComponent;
using osc::detail::NumComponents;
using osc::detail::StrideOf;
using osc::VertexAttributeFormat;

TEST(VertexAttributeHelpers, StrideOfWorks)
{
    static_assert(StrideOf(VertexAttributeFormat::Float32x2) == 2*sizeof(float));
}

TEST(VertexAttributeHelpers, NumComponentsWorks)
{
    static_assert(NumComponents(VertexAttributeFormat::Float32x3) == 3);
}

TEST(VertexAttributeHelpers, SizeOfComponentWorks)
{
    static_assert(SizeOfComponent(VertexAttributeFormat::Float32x3) == sizeof(float));
}

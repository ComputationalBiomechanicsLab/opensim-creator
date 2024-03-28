#include <oscar/Graphics/Detail/VertexAttributeFormatHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

using namespace osc;
using namespace osc::detail;

TEST(VertexAttributeHelpers, StrideOfWorks)
{
    static_assert(stride_of(VertexAttributeFormat::Float32x2) == 2*sizeof(float));
}

TEST(VertexAttributeHelpers, NumComponentsWorks)
{
    static_assert(num_components_in(VertexAttributeFormat::Float32x3) == 3);
}

TEST(VertexAttributeHelpers, SizeOfComponentWorks)
{
    static_assert(component_size(VertexAttributeFormat::Float32x3) == sizeof(float));
}

#include <oscar/Graphics/Detail/VertexAttributeFormatHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

using namespace osc;
using namespace osc::detail;

TEST(stride_of, WorksAsExpected)
{
    static_assert(stride_of(VertexAttributeFormat::Float32x2) == 2*sizeof(float));
}

TEST(num_components_in, WorksAsExpected)
{
    static_assert(num_components_in(VertexAttributeFormat::Float32x3) == 3);
}

TEST(component_size, WorksAsExpected)
{
    static_assert(component_size(VertexAttributeFormat::Float32x3) == sizeof(float));
}

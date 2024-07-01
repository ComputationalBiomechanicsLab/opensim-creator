#include <oscar/Graphics/Detail/VertexAttributeFormatHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

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

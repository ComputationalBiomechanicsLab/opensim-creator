#include <oscar/Graphics/DepthRenderBufferParams.h>

#include <gtest/gtest.h>
#include <oscar/Maths/Vec2.h>

using namespace osc;

TEST(DepthRenderBufferParams, can_default_construct)
{
    [[maybe_unused]] const DepthRenderBufferParams default_constructed;
}

TEST(DepthRenderBufferParams, default_constructed_has_1x1_dimensions)
{
    const DepthRenderBufferParams default_constructed;
    ASSERT_EQ(default_constructed.dimensions, Vec2i(1, 1));
}

TEST(DepthRenderBufferParams, has_value_equality)
{
    const DepthRenderBufferParams lhs;
    const DepthRenderBufferParams rhs;
    ASSERT_EQ(lhs, rhs);
}

TEST(DepthRenderBufferParams, changing_dimensions_changes_equality)
{
    const DepthRenderBufferParams lhs;
    const DepthRenderBufferParams rhs{.dimensions = {2, 2}};
    ASSERT_NE(lhs, rhs);
}

TEST(DepthRenderBufferParams, can_be_initialized_with_designated_initializer)
{
    [[maybe_unused]] const DepthRenderBufferParams params = {
        .dimensions = {2, 3},
        .dimensionality = TextureDimensionality::Cube,
        .format = DepthStencilFormat::D32_SFloat,
    };
}

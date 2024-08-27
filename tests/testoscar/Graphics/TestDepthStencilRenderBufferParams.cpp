#include <oscar/Graphics/DepthStencilRenderBufferParams.h>

#include <gtest/gtest.h>
#include <oscar/Maths/Vec2.h>

using namespace osc;

TEST(DepthStencilRenderBufferParams, can_default_construct)
{
    [[maybe_unused]] const DepthStencilRenderBufferParams default_constructed;
}

TEST(DepthStencilRenderBufferParams, default_constructed_has_1x1_dimensions)
{
    const DepthStencilRenderBufferParams default_constructed;
    ASSERT_EQ(default_constructed.dimensions, Vec2i(1, 1));
}

TEST(DepthStencilRenderBufferParams, has_value_equality)
{
    const DepthStencilRenderBufferParams lhs;
    const DepthStencilRenderBufferParams rhs;
    ASSERT_EQ(lhs, rhs);
}

TEST(DepthStencilRenderBufferParams, changing_dimensions_changes_equality)
{
    const DepthStencilRenderBufferParams lhs;
    const DepthStencilRenderBufferParams rhs{.dimensions = {2, 2}};
    ASSERT_NE(lhs, rhs);
}

TEST(DepthStencilRenderBufferParams, can_be_initialized_with_designated_initializer)
{
    [[maybe_unused]] const DepthStencilRenderBufferParams params = {
        .dimensions = {2, 3},
        .dimensionality = TextureDimensionality::Cube,
        .format = DepthStencilRenderBufferFormat::D32_SFloat,
    };
}

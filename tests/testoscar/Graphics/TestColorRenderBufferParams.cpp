#include <oscar/Graphics/ColorRenderBufferParams.h>

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/ColorRenderBufferFormat.h>
#include <oscar/Maths/Vec2.h>
#include <gtest/gtest.h>

using namespace osc;

TEST(ColorRenderBufferParams, can_default_construct)
{
    [[maybe_unused]] ColorRenderBufferParams default_constructed;
}

TEST(ColorRenderBufferParams, default_constructed_has_1x1_dimensions)
{
    const ColorRenderBufferParams default_constructed;
    ASSERT_EQ(default_constructed.dimensions, Vec2i(1, 1));
}

TEST(ColorRenderBufferParams, default_constructed_has_1x_AntiAliasingLevel)
{
    const ColorRenderBufferParams default_constructed;
    ASSERT_EQ(default_constructed.anti_aliasing_level, AntiAliasingLevel(1));
}

TEST(ColorRenderBufferParams, can_compare_for_equality)
{
    const ColorRenderBufferParams lhs;
    const ColorRenderBufferParams rhs;
    ASSERT_EQ(lhs, rhs) << "i.e. it's value equality";
}

TEST(ColorRenderBufferParams, changing_dimensions_changes_equality)
{
    const ColorRenderBufferParams lhs;
    const ColorRenderBufferParams rhs{.dimensions = {2, 2}};
    ASSERT_NE(lhs, rhs);
}

TEST(ColorRenderBufferParams, can_be_initialized_with_designated_initializer)
{
    const ColorRenderBufferParams params = {
        .dimensions = {2, 2},
        .format = ColorRenderBufferFormat::ARGB32,
    };

    ASSERT_EQ(params.dimensions, Vec2i(2, 2));
    ASSERT_EQ(params.format, ColorRenderBufferFormat::ARGB32);
}

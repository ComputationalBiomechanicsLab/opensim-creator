#include "render_pass_config.h"

#include <gtest/gtest.h>

#include <concepts>
#include <optional>

using namespace osc;

TEST(RenderPassConfig, defaults_to_no_pixel_rect_no_scissor_rect)
{
    const RenderPassConfig render_pass_config;
    ASSERT_EQ(render_pass_config.viewport_rect, std::nullopt);
    ASSERT_EQ(render_pass_config.scissor_rect, std::nullopt);
}

TEST(RenderPassConfig, is_regular)
{
    static_assert(std::regular<RenderPassConfig>);

    RenderPassConfig a;
    RenderPassConfig b;
    ASSERT_EQ(a, b);
    a.viewport_rect = Rect::from_origin_and_dimensions(Vector2{0.0f}, Vector2{10.0f});
    ASSERT_NE(a, b);
    b.viewport_rect = Rect::from_origin_and_dimensions(Vector2{0.0f}, Vector2{10.0f});
    ASSERT_EQ(a, b);
}

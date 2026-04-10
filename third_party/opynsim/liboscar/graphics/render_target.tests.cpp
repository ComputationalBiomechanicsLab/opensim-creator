#include "render_target.h"

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/color_render_buffer_params.h>
#include <liboscar/graphics/depth_stencil_render_buffer_params.h>
#include <liboscar/graphics/render_target_color_attachment.h>
#include <liboscar/graphics/render_texture.h>

#include <gtest/gtest.h>

#include <vector>

using namespace osc;

TEST(RenderTarget, can_default_construct)
{
    [[maybe_unused]] const RenderTarget default_constructed;
}

TEST(RenderTarget, default_constructed_has_one_dummy_color_attachment_and_one_dummy_depth_attachment)
{
    const RenderTarget default_constructed;
    ASSERT_EQ(default_constructed.color_attachments().size(), 1);
    ASSERT_TRUE(default_constructed.depth_attachment().has_value());
}

TEST(RenderTarget, can_construct_with_just_color_attachment)
{
    const SharedColorRenderBuffer buffer;
    const RenderTarget render_target{RenderTargetColorAttachment{.buffer = buffer}};

    ASSERT_EQ(render_target.color_attachments().size(), 1);
    ASSERT_EQ(render_target.color_attachments().front(), RenderTargetColorAttachment{.buffer = buffer});
    ASSERT_FALSE(render_target.depth_attachment().has_value());
}

TEST(RenderTarget, can_construct_with_just_depth_attachment)
{
    const SharedDepthStencilRenderBuffer depth_buffer;
    const RenderTarget render_target{RenderTargetDepthStencilAttachment{.buffer = depth_buffer}};

    ASSERT_TRUE(render_target.color_attachments().empty());
    ASSERT_TRUE(render_target.depth_attachment().has_value());
    ASSERT_EQ(*render_target.depth_attachment(), RenderTargetDepthStencilAttachment{.buffer = depth_buffer});
}

TEST(RenderTarget, can_construct_with_color_and_depth_attachments)
{
    const RenderTargetColorAttachment color_attachment;
    const RenderTargetDepthStencilAttachment depth_attachment;
    const RenderTarget render_target{color_attachment, depth_attachment};

    ASSERT_EQ(render_target.color_attachments().size(), 1);
    ASSERT_EQ(render_target.color_attachments().front(), color_attachment);
    ASSERT_EQ(render_target.depth_attachment(), depth_attachment);
}

TEST(RenderTarget, can_construct_with_2x_color_and_1x_depth_attachments)
{
    const RenderTargetColorAttachment color0_attachment;
    const RenderTargetColorAttachment color1_attachment{.clear_color = Color::red()};  // so they compare inequivalent
    const RenderTargetDepthStencilAttachment depth_attachment;
    const RenderTarget render_target{color0_attachment, color1_attachment, depth_attachment};

    ASSERT_NE(color0_attachment, color1_attachment);
    ASSERT_EQ(render_target.color_attachments().size(), 2);
    ASSERT_EQ(render_target.color_attachments()[0], color0_attachment);
    ASSERT_EQ(render_target.color_attachments()[1], color1_attachment);
    ASSERT_EQ(render_target.depth_attachment(), depth_attachment);
}

TEST(RenderTarget, can_construct_with_3x_color_and_1x_depth_attachments)
{
    const RenderTargetColorAttachment color0_attachment;
    const RenderTargetColorAttachment color1_attachment{.clear_color = Color::red()};  // so they compare inequivalent
    const RenderTargetColorAttachment color2_attachment{.clear_color = Color::green()};  // so they compare inequivalent
    const RenderTargetDepthStencilAttachment depth_attachment;
    const RenderTarget render_target{color0_attachment, color1_attachment, color2_attachment, depth_attachment};

    ASSERT_NE(color0_attachment, color1_attachment);
    ASSERT_NE(color1_attachment, color2_attachment);
    ASSERT_EQ(render_target.color_attachments().size(), 3);
    ASSERT_EQ(render_target.color_attachments()[0], color0_attachment);
    ASSERT_EQ(render_target.color_attachments()[1], color1_attachment);
    ASSERT_EQ(render_target.color_attachments()[2], color2_attachment);
    ASSERT_EQ(render_target.depth_attachment(), depth_attachment);
}

TEST(RenderTarget, validate_or_throw_doesnt_throw_when_given_buffers_with_same_dimensions_and_anti_aliasing_level)
{
    const SharedColorRenderBuffer color_buffer{ColorRenderBufferParams{.pixel_dimensions = Vector2i(3, 3), .anti_aliasing_level = AntiAliasingLevel{4}}};
    const RenderTargetColorAttachment color_attachment{.buffer = color_buffer};
    const SharedDepthStencilRenderBuffer depth_buffer{DepthStencilRenderBufferParams{.pixel_dimensions = Vector2i(3,3), .anti_aliasing_level = AntiAliasingLevel{4}}};
    const RenderTargetDepthStencilAttachment depth_attachment{.buffer = depth_buffer};

    const RenderTarget render_target{color_attachment, depth_attachment};
    ASSERT_NO_THROW({ render_target.validate_or_throw(); });
}

TEST(RenderTarget, default_constructed_has_device_pixel_ratio_of_1)
{
    ASSERT_EQ(RenderTarget{}.device_pixel_ratio(), 1.0f);
}

TEST(RenderTarget, set_device_pixel_ratio_sets_the_device_pixel_ratio)
{
    RenderTarget render_target;
    ASSERT_EQ(render_target.device_pixel_ratio(), 1.0f);
    render_target.set_device_pixel_ratio(2.0f);
    ASSERT_EQ(render_target.device_pixel_ratio(), 2.0f);
}

TEST(RenderTarget, default_constructed_as_1x1_dimensions)
{
    ASSERT_EQ(RenderTarget{}.dimensions(), Vector2(1.0f));
}

TEST(RenderTarget, setting_device_pixel_ratio_scales_dimensions)
{
    RenderTarget render_target;
    ASSERT_EQ(render_target.dimensions(), Vector2(1.0f));
    render_target.set_device_pixel_ratio(2.0f);
    ASSERT_EQ(render_target.dimensions(), Vector2(0.5f));
}

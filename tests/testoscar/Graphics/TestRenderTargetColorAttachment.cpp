#include <oscar/Graphics/RenderTargetColorAttachment.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTexture.h>

using namespace osc;

TEST(RenderTargetColorAttachment, can_construct_from_parts_of_a_RenderTexture)
{
    RenderTexture render_texture;
    RenderTargetColorAttachment attachment{
        render_texture.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    ASSERT_EQ(attachment.buffer, render_texture.upd_color_buffer());
    ASSERT_EQ(attachment.load_action, RenderBufferLoadAction::Clear);
    ASSERT_EQ(attachment.store_action, RenderBufferStoreAction::Resolve);
    ASSERT_EQ(attachment.clear_color, Color::red());
}

TEST(RenderTargetColorAttachment, compares_equal_to_its_copy)
{
    RenderTexture render_texture;
    RenderTargetColorAttachment attachment{
        render_texture.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };
    RenderTargetColorAttachment attachment_copy = attachment;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(attachment_copy, attachment);
}

TEST(RenderTargetColorAttachment, compares_equal_to_separately_constructed_instance_with_logically_equivalent_inputs)
{
    RenderTexture render_texture;

    RenderTargetColorAttachment attachment_a{
        render_texture.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };
    RenderTargetColorAttachment attachment_b{
        render_texture.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    ASSERT_EQ(attachment_a, attachment_b);
}

TEST(RenderTargetColorAttachment, compares_false_to_a_copy_after_copy_is_modified)
{
    RenderTexture first_render_texture;
    RenderTexture second_render_texture;
    RenderTargetColorAttachment attachment{
        first_render_texture.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    // modify buffer
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.buffer = second_render_texture.upd_color_buffer();
        ASSERT_NE(copy, attachment);
    }

    // modify load action
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.load_action = RenderBufferLoadAction::Load;
        ASSERT_NE(copy, attachment);
    }

    // modify store action
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.store_action = RenderBufferStoreAction::DontCare;
        ASSERT_NE(copy, attachment);
    }

    // modify color
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.clear_color = Color::green();
        ASSERT_NE(copy, attachment);
    }
}

#include <oscar/Graphics/RenderTargetDepthStencilAttachment.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTexture.h>

using namespace osc;

TEST(RenderTargetDepthStencilAttachment, can_construct_from_parts_of_a_RenderTexture)
{
    RenderTexture render_texture;

    RenderTargetDepthStencilAttachment attachment{
        render_texture.upd_depth_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    ASSERT_EQ(attachment.buffer, render_texture.upd_depth_buffer());
    ASSERT_EQ(attachment.load_action, RenderBufferLoadAction::Clear);
    ASSERT_EQ(attachment.store_action, RenderBufferStoreAction::Resolve);
}

TEST(RenderTargetDepthStencilAttachment, compares_equal_to_copies)
{
    RenderTexture render_texture;
    RenderTargetDepthStencilAttachment attachment{
        render_texture.upd_depth_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };
    const RenderTargetDepthStencilAttachment copy = attachment;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(copy, attachment);
}

TEST(RenderTargetDepthStencilAttachment, compares_equal_to_separately_constructed_but_logically_equal_value)
{
    RenderTexture render_texture;

    RenderTargetDepthStencilAttachment a{
        render_texture.upd_depth_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };
    RenderTargetDepthStencilAttachment b{
        render_texture.upd_depth_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    ASSERT_EQ(a, b);
}

TEST(RenderTargetDepthStencilAttachment, compares_false_if_something_in_a_copy_is_modified)
{
    RenderTexture first_render_texture;
    RenderTexture second_render_texture;
    RenderTargetDepthStencilAttachment attachment{
        first_render_texture.upd_depth_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    // modify buffer
    {
        RenderTargetDepthStencilAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.buffer = second_render_texture.upd_depth_buffer();
        ASSERT_NE(copy, attachment);
    }

    // modify load action
    {
        RenderTargetDepthStencilAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.load_action = RenderBufferLoadAction::Load;
        ASSERT_NE(copy, attachment);
    }

    // modify store action
    {
        RenderTargetDepthStencilAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.store_action = RenderBufferStoreAction::DontCare;
        ASSERT_NE(copy, attachment);
    }
}

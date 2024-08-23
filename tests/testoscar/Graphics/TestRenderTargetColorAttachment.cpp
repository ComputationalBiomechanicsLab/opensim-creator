#include <oscar/Graphics/RenderTargetColorAttachment.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTexture.h>

using namespace osc;

TEST(RenderTargetColorAttachment, CanConstructFromPartsOfRenderTexture)
{
    RenderTexture renderTex;
    RenderTargetColorAttachment attachment
    {
        renderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    ASSERT_EQ(attachment.color_buffer, renderTex.upd_color_buffer());
    ASSERT_EQ(attachment.load_action, RenderBufferLoadAction::Clear);
    ASSERT_EQ(attachment.store_action, RenderBufferStoreAction::Resolve);
    ASSERT_EQ(attachment.clear_color, Color::red());
}

TEST(RenderTargetColorAttachment, EqualityReturnsTrueForCopies)
{
    RenderTexture renderTex;
    RenderTargetColorAttachment attachment
    {
        renderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };
    RenderTargetColorAttachment copy = attachment;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(copy, attachment);
}

TEST(RenderTargetColorAttachment, EqualityReturnsTrueForSeperatelyConstructedButLogicallyEqualValues)
{
    RenderTexture renderTex;

    RenderTargetColorAttachment a
    {
        renderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    RenderTargetColorAttachment b
    {
        renderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    ASSERT_EQ(a, b);
}

TEST(RenderTargetColorAttachment, EqualityReturnsFalseIfSomethingIsModified)
{
    RenderTexture firstRenderTex;
    RenderTexture secondRenderTex;
    RenderTargetColorAttachment attachment
    {
        firstRenderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    // modify buffer
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.color_buffer = secondRenderTex.upd_color_buffer();
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

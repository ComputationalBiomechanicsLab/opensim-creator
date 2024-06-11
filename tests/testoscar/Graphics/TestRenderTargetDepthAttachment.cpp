#include <oscar/Graphics/RenderTargetDepthAttachment.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTexture.h>

using namespace osc;

TEST(RenderTargetDepthAttachment, CanConstructFromPartsOfRenderTexture)
{
    RenderTexture renderTex{{1, 1}};

    RenderTargetDepthAttachment attachment
    {
        renderTex.upd_depth_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    ASSERT_EQ(attachment.buffer, renderTex.upd_depth_buffer());
    ASSERT_EQ(attachment.load_action, RenderBufferLoadAction::Clear);
    ASSERT_EQ(attachment.store_action, RenderBufferStoreAction::Resolve);
}

TEST(RenderTargetDepthAttachment, ConstructingWithNullptrThrowsException)
{
    ASSERT_ANY_THROW(
    {
        RenderTargetDepthAttachment
        (
            nullptr,
            RenderBufferLoadAction::Clear,
            RenderBufferStoreAction::Resolve
        );
    });
}

TEST(RenderTargetDepthAttachment, EqualityReturnsTrueForCopies)
{
    RenderTexture renderTex;
    RenderTargetDepthAttachment attachment
    {
        renderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };
    const RenderTargetDepthAttachment copy = attachment;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(copy, attachment);
}

TEST(RenderTargetDepthAttachment, EqualityReturnsTrueForSeperatelyConstructedButLogicallyEqualValues)
{
    RenderTexture renderTex;

    RenderTargetDepthAttachment a
    {
        renderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    RenderTargetDepthAttachment b
    {
        renderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    ASSERT_EQ(a, b);
}

TEST(RenderTargetDepthAttachment, EqualityReturnsFalseIfSomethingIsModified)
{
    RenderTexture firstRenderTex;
    RenderTexture secondRenderTex;
    RenderTargetDepthAttachment attachment
    {
        firstRenderTex.upd_color_buffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    // modify buffer
    {
        RenderTargetDepthAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.buffer = secondRenderTex.upd_color_buffer();
        ASSERT_NE(copy, attachment);
    }

    // modify load action
    {
        RenderTargetDepthAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.load_action = RenderBufferLoadAction::Load;
        ASSERT_NE(copy, attachment);
    }

    // modify store action
    {
        RenderTargetDepthAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.store_action = RenderBufferStoreAction::DontCare;
        ASSERT_NE(copy, attachment);
    }
}

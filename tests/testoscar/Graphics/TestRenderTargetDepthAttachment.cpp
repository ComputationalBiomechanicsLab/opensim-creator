#include <oscar/Graphics/RenderTargetDepthAttachment.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/RenderBufferLoadAction.hpp>
#include <oscar/Graphics/RenderBufferStoreAction.hpp>
#include <oscar/Graphics/RenderTexture.hpp>

using osc::RenderBufferLoadAction;
using osc::RenderBufferStoreAction;
using osc::RenderTargetDepthAttachment;
using osc::RenderTexture;

TEST(RenderTargetDepthAttachment, CanConstructFromPartsOfRenderTexture)
{
    RenderTexture renderTex{{1, 1}};

    RenderTargetDepthAttachment attachment
    {
        renderTex.updDepthBuffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    ASSERT_EQ(attachment.buffer, renderTex.updDepthBuffer());
    ASSERT_EQ(attachment.loadAction, RenderBufferLoadAction::Clear);
    ASSERT_EQ(attachment.storeAction, RenderBufferStoreAction::Resolve);
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
        renderTex.updColorBuffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };
    RenderTargetDepthAttachment const copy = attachment;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(copy, attachment);
}

TEST(RenderTargetDepthAttachment, EqualityReturnsTrueForSeperatelyConstructedButLogicallyEqualValues)
{
    RenderTexture renderTex;

    RenderTargetDepthAttachment a
    {
        renderTex.updColorBuffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    RenderTargetDepthAttachment b
    {
        renderTex.updColorBuffer(),
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
        firstRenderTex.updColorBuffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    // modify buffer
    {
        RenderTargetDepthAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.buffer = secondRenderTex.updColorBuffer();
        ASSERT_NE(copy, attachment);
    }

    // modify load action
    {
        RenderTargetDepthAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.loadAction = RenderBufferLoadAction::Load;
        ASSERT_NE(copy, attachment);
    }

    // modify store action
    {
        RenderTargetDepthAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.storeAction = RenderBufferStoreAction::DontCare;
        ASSERT_NE(copy, attachment);
    }
}

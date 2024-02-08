#include <oscar/Graphics/RenderTargetColorAttachment.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/RenderBufferLoadAction.hpp>
#include <oscar/Graphics/RenderBufferStoreAction.hpp>
#include <oscar/Graphics/RenderTexture.hpp>

using osc::RenderTexture;
using osc::RenderTargetColorAttachment;
using osc::RenderBufferLoadAction;
using osc::RenderBufferStoreAction;
using osc::Color;

TEST(RenderTargetColorAttachment, CanConstructFromPartsOfRenderTexture)
{
    RenderTexture renderTex;
    RenderTargetColorAttachment attachment
    {
        renderTex.updColorBuffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    ASSERT_EQ(attachment.buffer, renderTex.updColorBuffer());
    ASSERT_EQ(attachment.loadAction, RenderBufferLoadAction::Clear);
    ASSERT_EQ(attachment.storeAction, RenderBufferStoreAction::Resolve);
    ASSERT_EQ(attachment.clearColor, Color::red());
}

TEST(RenderTargetColorAttachment, ConstructingWithNullptrThrowsException)
{
    ASSERT_ANY_THROW(
    {
        RenderTargetColorAttachment
        (
            nullptr,
            RenderBufferLoadAction::Clear,
            RenderBufferStoreAction::Resolve,
            Color::red()
        );
    });
}

TEST(RenderTargetColorAttachment, EqualityReturnsTrueForCopies)
{
    RenderTexture renderTex;
    RenderTargetColorAttachment attachment
    {
        renderTex.updColorBuffer(),
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
        renderTex.updColorBuffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    RenderTargetColorAttachment b
    {
        renderTex.updColorBuffer(),
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
        firstRenderTex.updColorBuffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
        Color::red(),
    };

    // modify buffer
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.buffer = secondRenderTex.updColorBuffer();
        ASSERT_NE(copy, attachment);
    }

    // modify load action
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.loadAction = RenderBufferLoadAction::Load;
        ASSERT_NE(copy, attachment);
    }

    // modify store action
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.storeAction = RenderBufferStoreAction::DontCare;
        ASSERT_NE(copy, attachment);
    }

    // modify color
    {
        RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.clearColor = Color::green();
        ASSERT_NE(copy, attachment);
    }
}

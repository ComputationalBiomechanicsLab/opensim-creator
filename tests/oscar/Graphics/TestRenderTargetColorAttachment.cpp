#include "oscar/Graphics/RenderTargetColorAttachment.hpp"

#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/RenderBufferLoadAction.hpp"
#include "oscar/Graphics/RenderBufferStoreAction.hpp"
#include "oscar/Graphics/RenderTexture.hpp"

#include <gtest/gtest.h>

TEST(RenderTargetColorAttachment, CanConstructFromPartsOfRenderTexture)
{
    osc::RenderTexture renderTex;
    osc::RenderTargetColorAttachment attachment
    {
        renderTex.updColorBuffer(),
        osc::RenderBufferLoadAction::Clear,
        osc::RenderBufferStoreAction::Resolve,
        osc::Color::red(),
    };

    ASSERT_EQ(attachment.buffer, renderTex.updColorBuffer());
    ASSERT_EQ(attachment.loadAction, osc::RenderBufferLoadAction::Clear);
    ASSERT_EQ(attachment.storeAction, osc::RenderBufferStoreAction::Resolve);
    ASSERT_EQ(attachment.clearColor, osc::Color::red());
}

TEST(RenderTargetColorAttachment, ConstructingWithNullptrThrowsException)
{
    ASSERT_ANY_THROW(
    {
        osc::RenderTargetColorAttachment
        (
            nullptr,
            osc::RenderBufferLoadAction::Clear,
            osc::RenderBufferStoreAction::Resolve,
            osc::Color::red()
        );
    });
}

TEST(RenderTargetColorAttachment, EqualityReturnsTrueForCopies)
{
    osc::RenderTexture renderTex;
    osc::RenderTargetColorAttachment attachment
    {
        renderTex.updColorBuffer(),
        osc::RenderBufferLoadAction::Clear,
        osc::RenderBufferStoreAction::Resolve,
        osc::Color::red(),
    };
    osc::RenderTargetColorAttachment copy = attachment;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(copy, attachment);
}

TEST(RenderTargetColorAttachment, EqualityReturnsTrueForSeperatelyConstructedButLogicallyEqualValues)
{
    osc::RenderTexture renderTex;

    osc::RenderTargetColorAttachment a
    {
        renderTex.updColorBuffer(),
        osc::RenderBufferLoadAction::Clear,
        osc::RenderBufferStoreAction::Resolve,
        osc::Color::red(),
    };

    osc::RenderTargetColorAttachment b
    {
        renderTex.updColorBuffer(),
        osc::RenderBufferLoadAction::Clear,
        osc::RenderBufferStoreAction::Resolve,
        osc::Color::red(),
    };

    ASSERT_EQ(a, b);
}

TEST(RenderTargetColorAttachment, EqualityReturnsFalseIfSomethingIsModified)
{
    osc::RenderTexture firstRenderTex;
    osc::RenderTexture secondRenderTex;
    osc::RenderTargetColorAttachment attachment
    {
        firstRenderTex.updColorBuffer(),
        osc::RenderBufferLoadAction::Clear,
        osc::RenderBufferStoreAction::Resolve,
        osc::Color::red(),
    };

    // modify buffer
    {
        osc::RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.buffer = secondRenderTex.updColorBuffer();
        ASSERT_NE(copy, attachment);
    }

    // modify load action
    {
        osc::RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.loadAction = osc::RenderBufferLoadAction::Load;
        ASSERT_NE(copy, attachment);
    }

    // modify store action
    {
        osc::RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.storeAction = osc::RenderBufferStoreAction::DontCare;
        ASSERT_NE(copy, attachment);
    }

    // modify color
    {
        osc::RenderTargetColorAttachment copy = attachment;
        ASSERT_EQ(copy, attachment);
        copy.clearColor = osc::Color::green();
        ASSERT_NE(copy, attachment);
    }
}

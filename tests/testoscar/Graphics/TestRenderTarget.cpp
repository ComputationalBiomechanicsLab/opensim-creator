#include <oscar/Graphics/RenderTarget.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTargetColorAttachment.h>
#include <oscar/Graphics/RenderTexture.h>

#include <vector>

using namespace osc;

TEST(RenderTarget, CtorWorksAsExpected)
{
    RenderTexture renderTexture;

    std::vector<RenderTargetColorAttachment> colors =
    {
        RenderTargetColorAttachment
        {
            renderTexture.updColorBuffer(),
            RenderBufferLoadAction::Load,
            RenderBufferStoreAction::DontCare,
            Color::blue(),
        },
    };

    RenderTargetDepthAttachment depth
    {
        renderTexture.updDepthBuffer(),
        RenderBufferLoadAction::Clear,
        RenderBufferStoreAction::Resolve,
    };

    RenderTarget target{colors, depth};

    ASSERT_EQ(target.colors, colors);
    ASSERT_EQ(target.depth, depth);
}

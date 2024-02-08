#include <oscar/Graphics/RenderTarget.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/RenderBufferLoadAction.hpp>
#include <oscar/Graphics/RenderBufferStoreAction.hpp>
#include <oscar/Graphics/RenderTargetColorAttachment.hpp>
#include <oscar/Graphics/RenderTexture.hpp>

#include <vector>

using osc::Color;
using osc::RenderBufferLoadAction;
using osc::RenderBufferStoreAction;
using osc::RenderTarget;
using osc::RenderTargetColorAttachment;
using osc::RenderTargetDepthAttachment;
using osc::RenderTexture;

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

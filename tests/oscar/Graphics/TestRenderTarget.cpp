#include "oscar/Graphics/RenderTarget.hpp"

#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/RenderBufferLoadAction.hpp"
#include "oscar/Graphics/RenderBufferStoreAction.hpp"
#include "oscar/Graphics/RenderTexture.hpp"
#include "oscar/Graphics/RenderTargetColorAttachment.hpp"

#include <gtest/gtest.h>

#include <vector>

TEST(RenderTarget, CtorWorksAsExpected)
{
    osc::RenderTexture renderTexture;

    std::vector<osc::RenderTargetColorAttachment> colors =
    {
        osc::RenderTargetColorAttachment
        {
            renderTexture.updColorBuffer(),
            osc::RenderBufferLoadAction::Load,
            osc::RenderBufferStoreAction::DontCare,
            osc::Color::blue(),
        },
    };

    osc::RenderTargetDepthAttachment depth
    {
        renderTexture.updDepthBuffer(),
        osc::RenderBufferLoadAction::Clear,
        osc::RenderBufferStoreAction::Resolve,
    };

    osc::RenderTarget target{colors, depth};

    ASSERT_EQ(target.colors, colors);
    ASSERT_EQ(target.depth, depth);
}

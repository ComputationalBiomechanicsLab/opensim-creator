#include "src/Graphics/RenderTarget.hpp"

#include "src/Graphics/Color.hpp"
#include "src/Graphics/RenderBufferLoadAction.hpp"
#include "src/Graphics/RenderBufferStoreAction.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/RenderTargetColorAttachment.hpp"

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
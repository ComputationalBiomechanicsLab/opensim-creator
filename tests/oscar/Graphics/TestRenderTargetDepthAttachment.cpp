#include "oscar/Graphics/RenderTargetDepthAttachment.hpp"

#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/RenderBufferLoadAction.hpp"
#include "oscar/Graphics/RenderBufferStoreAction.hpp"
#include "oscar/Graphics/RenderTexture.hpp"

#include <gtest/gtest.h>

TEST(RenderTargetDepthAttachment, CanConstructFromPartsOfRenderTexture)
{
	osc::RenderTexture renderTex{{1, 1}};

	osc::RenderTargetDepthAttachment attachment
	{
		renderTex.updDepthBuffer(),
		osc::RenderBufferLoadAction::Clear,
		osc::RenderBufferStoreAction::Resolve,
	};

	ASSERT_EQ(attachment.buffer, renderTex.updDepthBuffer());
	ASSERT_EQ(attachment.loadAction, osc::RenderBufferLoadAction::Clear);
	ASSERT_EQ(attachment.storeAction, osc::RenderBufferStoreAction::Resolve);
}

TEST(RenderTargetDepthAttachment, ConstructingWithNullptrThrowsException)
{
	ASSERT_ANY_THROW(
	{
		osc::RenderTargetDepthAttachment
		(
			nullptr,
			osc::RenderBufferLoadAction::Clear,
			osc::RenderBufferStoreAction::Resolve
		);
	});
}

TEST(RenderTargetDepthAttachment, EqualityReturnsTrueForCopies)
{
	osc::RenderTexture renderTex;
	osc::RenderTargetDepthAttachment attachment
	{
		renderTex.updColorBuffer(),
		osc::RenderBufferLoadAction::Clear,
		osc::RenderBufferStoreAction::Resolve,
	};
	osc::RenderTargetDepthAttachment const copy = attachment;  // NOLINT(performance-unnecessary-copy-initialization)

	ASSERT_EQ(copy, attachment);
}

TEST(RenderTargetDepthAttachment, EqualityReturnsTrueForSeperatelyConstructedButLogicallyEqualValues)
{
	osc::RenderTexture renderTex;

	osc::RenderTargetDepthAttachment a
	{
		renderTex.updColorBuffer(),
		osc::RenderBufferLoadAction::Clear,
		osc::RenderBufferStoreAction::Resolve,
	};

	osc::RenderTargetDepthAttachment b
	{
		renderTex.updColorBuffer(),
		osc::RenderBufferLoadAction::Clear,
		osc::RenderBufferStoreAction::Resolve,
	};

	ASSERT_EQ(a, b);
}

TEST(RenderTargetDepthAttachment, EqualityReturnsFalseIfSomethingIsModified)
{
	osc::RenderTexture firstRenderTex;
	osc::RenderTexture secondRenderTex;
	osc::RenderTargetDepthAttachment attachment
	{
		firstRenderTex.updColorBuffer(),
		osc::RenderBufferLoadAction::Clear,
		osc::RenderBufferStoreAction::Resolve,
	};

	// modify buffer
	{
		osc::RenderTargetDepthAttachment copy = attachment;
		ASSERT_EQ(copy, attachment);
		copy.buffer = secondRenderTex.updColorBuffer();
		ASSERT_NE(copy, attachment);
	}

	// modify load action
	{
		osc::RenderTargetDepthAttachment copy = attachment;
		ASSERT_EQ(copy, attachment);
		copy.loadAction = osc::RenderBufferLoadAction::Load;
		ASSERT_NE(copy, attachment);
	}

	// modify store action
	{
		osc::RenderTargetDepthAttachment copy = attachment;
		ASSERT_EQ(copy, attachment);
		copy.storeAction = osc::RenderBufferStoreAction::DontCare;
		ASSERT_NE(copy, attachment);
	}
}

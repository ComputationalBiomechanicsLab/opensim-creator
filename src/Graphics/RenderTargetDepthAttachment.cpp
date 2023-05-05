#include "RenderTargetDepthAttachment.hpp"

bool osc::operator==(RenderTargetDepthAttachment const& a, RenderTargetDepthAttachment const& b) noexcept
{
	return
		a.buffer == b.buffer &&
		a.loadAction == b.loadAction &&
		a.storeAction == b.storeAction;
}

bool osc::operator!=(RenderTargetDepthAttachment const& a, RenderTargetDepthAttachment const& b) noexcept
{
	return !(a == b);
}
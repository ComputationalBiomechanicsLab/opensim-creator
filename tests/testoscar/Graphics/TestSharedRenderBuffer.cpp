#include <oscar/Graphics/SharedRenderBuffer.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(SharedRenderBuffer, can_default_construct)
{
    [[maybe_unused]] const SharedRenderBuffer default_constructed;
}

TEST(SharedRenderBuffer, can_construct_depth_buffer)
{
    [[maybe_unused]] const SharedRenderBuffer depth_buffer{RenderTextureParams{}, RenderBufferType::Depth};
}

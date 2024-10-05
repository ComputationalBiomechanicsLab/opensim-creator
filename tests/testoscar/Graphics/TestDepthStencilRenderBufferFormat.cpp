#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>
#include <sstream>

using namespace osc;

TEST(DepthStencilRenderBufferFormat, can_be_iterated_over_and_streamed_to_ostream)
{
    for (size_t i = 0; i < num_options<DepthStencilRenderBufferFormat>(); ++i)
    {
        std::stringstream ss;
        ss << static_cast<DepthStencilRenderBufferFormat>(i);  // shouldn't throw
    }
}

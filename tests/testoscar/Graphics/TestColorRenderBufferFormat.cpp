#include <oscar/Graphics/ColorRenderBufferFormat.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <sstream>
#include <string>
#include <utility>

using namespace osc;

TEST(ColorRenderBufferFormat, any_value_can_be_written_to_an_ostream)
{
    for (size_t i = 0; i < num_options<ColorRenderBufferFormat>(); ++i) {
        const auto format = static_cast<ColorRenderBufferFormat>(i);
        std::stringstream ss;
        ss << format;

        ASSERT_FALSE(std::move(ss).str().empty());
    }
}

TEST(ColorRenderBufferFormat, defaults_to_R8G8B8A8_SRGB)
{
    static_assert(ColorRenderBufferFormat::Default == ColorRenderBufferFormat::R8G8B8A8_SRGB);
}

TEST(ColorRenderBufferFormat, DefaultHDR_is_R16R16B16A16_SFLOAT)
{
    static_assert(ColorRenderBufferFormat::DefaultHDR == ColorRenderBufferFormat::R16G16B16A16_SFLOAT);
}

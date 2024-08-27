#include <oscar/Graphics/ColorRenderBufferFormat.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <sstream>
#include <string>
#include <utility>

using namespace osc;

TEST(ColorRenderBufferFormat, AnyValueCanBePrintedToStream)
{
    for (size_t i = 0; i < num_options<ColorRenderBufferFormat>(); ++i) {
        const auto format = static_cast<ColorRenderBufferFormat>(i);
        std::stringstream ss;
        ss << format;

        ASSERT_FALSE(std::move(ss).str().empty());
    }
}

TEST(ColorRenderBufferFormat, DefaultsToRGBA32)
{
    static_assert(ColorRenderBufferFormat::Default == ColorRenderBufferFormat::ARGB32);
}

TEST(ColorRenderBufferFormat, DefaultHDRFormatIsARGBHalf)
{
    static_assert(ColorRenderBufferFormat::DefaultHDR == ColorRenderBufferFormat::ARGBFloat16);
}

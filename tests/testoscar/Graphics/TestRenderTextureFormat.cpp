#include <oscar/Graphics/RenderTextureFormat.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <sstream>
#include <string>
#include <utility>

using namespace osc;

TEST(RenderTextureFormat, AnyValueCanBePrintedToStream)
{
    for (size_t i = 0; i < NumOptions<RenderTextureFormat>(); ++i)
    {
        auto const format = static_cast<RenderTextureFormat>(i);
        std::stringstream ss;
        ss << format;

        ASSERT_FALSE(std::move(ss).str().empty());
    }
}

TEST(RenderTextureFormat, DefaultsToRGBA32)
{
    static_assert(RenderTextureFormat::Default == RenderTextureFormat::ARGB32);
}

TEST(RenderTextureFormat, DefaultHDRFormatIsARGBHalf)
{
    static_assert(RenderTextureFormat::DefaultHDR == RenderTextureFormat::ARGBFloat16);
}

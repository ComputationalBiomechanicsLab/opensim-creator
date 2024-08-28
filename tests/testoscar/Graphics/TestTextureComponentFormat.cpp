#include <oscar/Graphics/TextureComponentFormat.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

using namespace osc;

TEST(TextureComponentFormat, num_bytes_per_component_in_returns_expected_values)
{
    static_assert(num_options<TextureComponentFormat>() == 2);

    ASSERT_EQ(num_bytes_per_component_in(TextureComponentFormat::Uint8), 1);
    ASSERT_EQ(num_bytes_per_component_in(TextureComponentFormat::Float32), 4);
}

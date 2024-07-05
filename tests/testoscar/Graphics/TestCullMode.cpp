#include <oscar/Graphics/CullMode.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(CullMode, Default_is_Off)
{
    static_assert(CullMode::Default == CullMode::Off);
}

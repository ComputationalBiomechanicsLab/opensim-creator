#include <oscar/Graphics/CullMode.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(CullMode, DefaultsToOff)
{
    static_assert(CullMode::Default == CullMode::Off);
}

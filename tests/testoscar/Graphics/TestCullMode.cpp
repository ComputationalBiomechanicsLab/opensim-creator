#include <oscar/Graphics/CullMode.hpp>

#include <gtest/gtest.h>

using osc::CullMode;

TEST(CullMode, DefaultsToOff)
{
    static_assert(CullMode::Default == CullMode::Off);
}

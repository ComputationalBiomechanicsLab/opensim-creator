#include "oscar/Graphics/CullMode.hpp"

#include <gtest/gtest.h>

TEST(CullMode, DefaultsToOff)
{
    static_assert(osc::CullMode::Default == osc::CullMode::Off);
}

#include <oscar/Maths/FrustumPlanes.h>

#include <gtest/gtest.h>

#include <concepts>

using namespace osc;

TEST(FrustumPlanes, IsRegular)
{
    static_assert(std::regular<FrustumPlanes>);
}

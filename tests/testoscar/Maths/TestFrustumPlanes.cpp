#include <oscar/Maths/FrustumPlanes.h>

#include <gtest/gtest.h>

#include <concepts>

using namespace osc;

TEST(FrustumPlanes, is_regular)
{
    static_assert(std::regular<FrustumPlanes>);
}

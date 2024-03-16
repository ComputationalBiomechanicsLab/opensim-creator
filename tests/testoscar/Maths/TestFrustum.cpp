#include <oscar/Maths/Frustum.h>

#include <gtest/gtest.h>

#include <concepts>

using namespace osc;

TEST(Frustum, IsRegular)
{
    static_assert(std::regular<Frustum>);
}

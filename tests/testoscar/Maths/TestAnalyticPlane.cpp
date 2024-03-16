#include <oscar/Maths/AnalyticPlane.h>

#include <gtest/gtest.h>

#include <concepts>
#include <sstream>

using namespace osc;

TEST(AnalyticPlane, IsRegular)
{
    static_assert(std::regular<AnalyticPlane>);
}

TEST(AnalyticPlane, ConstructsPointingUpwards)
{
    static_assert(AnalyticPlane{}.normal == Vec3{0.0f, 1.0f, 0.0f});
}

TEST(AnalyticPlane, CanStreamToString)
{
    std::stringstream ss;
    ss << AnalyticPlane{};
    ASSERT_FALSE(ss.str().empty());
}

#include <oscar/Maths/AnalyticPlane.h>

#include <gtest/gtest.h>

#include <concepts>
#include <sstream>

using namespace osc;

TEST(AnalyticPlane, is_regular)
{
    static_assert(std::regular<AnalyticPlane>);
}

TEST(AnalyticPlane, default_constructor_normal_points_along_plus_y)
{
    static_assert(AnalyticPlane{}.normal == Vec3{0.0f, 1.0f, 0.0f});
}

TEST(AnalyticPlane, can_be_written_to_std_ostream)
{
    std::stringstream ss;
    ss << AnalyticPlane{};
    ASSERT_FALSE(ss.str().empty());
}

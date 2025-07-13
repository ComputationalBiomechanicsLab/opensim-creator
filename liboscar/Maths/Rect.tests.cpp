#include "Rect.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(Rect, of_point_origin_is_the_point)
{
    ASSERT_EQ(Rect::of_point(Vec2{3.0f}).origin(), Vec2{3.0f});
}

TEST(Rect, of_point_returns_zero_dimension_Rect)
{
    ASSERT_EQ(Rect::of_point(Vec2{3.0f}).dimensions(), Vec2{});
}

TEST(Rect, from_origin_and_dimensions)
{
    const Vec2 origin = {-5.0f, -10.0f};
    const Vec2 dimensions = {3.0f, 11.0f};

    const Rect result = Rect::from_origin_and_dimensions(origin, dimensions);

    ASSERT_EQ(result.origin(), origin);
    ASSERT_EQ(result.dimensions(), dimensions);
}

TEST(Rect, dimensions_returns_expected_dimensions)
{
    const Rect rect{Vec2(-9.0f, 3.0f), Vec2(-13.0f, 9.0f)};

    ASSERT_EQ(rect.dimensions(), Vec2(4.0f, 6.0f));
}

TEST(Rect, half_extents_returns_expected_half_dimensions)
{
    const Rect rect{Vec2(-9.0f, 3.0f), Vec2(-13.0f, 9.0f)};

    ASSERT_EQ(rect.half_extents(), Vec2(2.0f, 3.0f));
}

TEST(Rect, area_returns_expected_area)
{
    const Rect rect{Vec2{5.0f, 3.0f}, Vec2{6.0f, 5.0f}};

    ASSERT_EQ(rect.area(), 2.0f);
}

TEST(Rect, corner_to_corner_constructor_works_with_min_max_righthanded_corners)
{
    const Rect rect{Vec2{-1.0f}, Vec2{+1.0f}};

    ASSERT_EQ(rect.origin(), Vec2{});
    ASSERT_EQ(rect.dimensions(), Vec2{2.0f});
}

TEST(Rect, corners_returns_min_and_max_corners_of_the_rect)
{
    const Rect rect{Vec2{-1.0f}, Vec2{+1.0f}};

    const auto corners = rect.corners();

    ASSERT_EQ(corners.min, Vec2{-1.0f});
    ASSERT_EQ(corners.max, Vec2{+1.0f});
}

TEST(Rect, min_corner_returns_expected_result)
{
    const Rect rect{Vec2{-7.0f}, Vec2{+3.0f}};

    ASSERT_EQ(rect.min_corner(), Vec2{-7.0f});
}

TEST(Rect, max_corner_returns_expected_result)
{
    const Rect rect{Vec2{-7.0f}, Vec2{+3.0f}};

    ASSERT_EQ(rect.max_corner(), Vec2{+3.0f});
}

TEST(Rect, ypd_top_left_returns_expected_point)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    ASSERT_EQ(rect.ypd_top_left(), Vec2(5.0f));
}

TEST(Rect, ypd_top_right_returns_expected_point)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    ASSERT_EQ(rect.ypd_top_right(), Vec2(50.0f, 5.0f));
}

TEST(Rect, ypd_bottom_left_returns_expected_point)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    ASSERT_EQ(rect.ypd_bottom_left(), Vec2(5.0f, 50.0f));
}

TEST(Rect, ypd_bottom_right_returns_expected_point)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    ASSERT_EQ(rect.ypd_bottom_right(), Vec2(50.0f, 50.0f));
}

TEST(Rect, ypu_top_left_returns_expected_point)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    ASSERT_EQ(rect.ypu_top_left(), Vec2(5.0f, 50.0f));
}

TEST(Rect, ypu_top_right_returns_expected_point)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    ASSERT_EQ(rect.ypu_top_right(), Vec2(50.0f));
}

TEST(Rect, ypu_bottom_left_returns_expected_point)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    ASSERT_EQ(rect.ypu_bottom_left(), Vec2(5.0f));
}

TEST(Rect, ypu_bottom_right_returns_expected_point)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    ASSERT_EQ(rect.ypu_bottom_right(), Vec2(50.0f, 5.0f));
}

TEST(Rect, with_dimensions_scaled_by_rescales_dimensions)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    const Rect result = rect.with_dimensions_scaled_by({0.5f, 2.25f});

    ASSERT_EQ(result.dimensions(), Vec2(0.5f*45.0f, 2.25f*45.0f));
}

TEST(Rect, with_dimensions_scaled_by_doesnt_change_origin)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    const Rect result = rect.with_dimensions_scaled_by({0.5f, 2.25f});

    ASSERT_EQ(result.origin(), rect.origin());
}

TEST(Rect, with_origin_and_dimensions_scaled_by_rescales_both_origin_and_dimensions)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    const Rect result = rect.with_origin_and_dimensions_scaled_by(0.5f);

    ASSERT_EQ(result.dimensions(), 0.5f*rect.dimensions());
    ASSERT_EQ(result.origin(), 0.5f*rect.origin());
}

TEST(Rect, with_flipped_y_returns_expected_rect)
{
    const Rect rect{Vec2{5.0f}, Vec2{50.0f}};

    const Rect result = rect.with_flipped_y(125.0f);
    const auto corners = result.corners();

    ASSERT_EQ(corners.min.x, 5.0f);
    ASSERT_EQ(corners.min.y, 75.0f);
    ASSERT_EQ(corners.max.x, 50.0f);
    ASSERT_EQ(corners.max.y, 120.0f);
}

TEST(Rect, expanded_by_float_adds_float_to_dimensions)
{
    const Rect rect{Vec2{-1.0f}, Vec2{+1.0f}};

    const Rect result = rect.expanded_by(1.0f);

    ASSERT_EQ(result.dimensions(), Vec2{4.0f});
}


TEST(Rect, expanded_by_Vec2_adds_to_each_part_of_dimensions)
{
    const Rect rect{Vec2{-1.0f}, Vec2{+1.0f}};

    const Rect result = rect.expanded_by(Vec2{1.0f, 0.5f});

    ASSERT_EQ(result.dimensions(), Vec2(4.0f, 3.0f));
}

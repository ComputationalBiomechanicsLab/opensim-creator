#include "rect.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(Rect, from_point_origin_is_the_point)
{
    ASSERT_EQ(Rect::from_point(Vector2{3.0f}).origin(), Vector2{3.0f});
}

TEST(Rect, from_point_returns_zero_dimension_Rect)
{
    ASSERT_EQ(Rect::from_point(Vector2{3.0f}).dimensions(), Vector2{});
}

TEST(Rect, from_origin_and_dimensions)
{
    const Vector2 origin = {-5.0f, -10.0f};
    const Vector2 dimensions = {3.0f, 11.0f};

    const Rect result = Rect::from_origin_and_dimensions(origin, dimensions);

    ASSERT_EQ(result.origin(), origin);
    ASSERT_EQ(result.dimensions(), dimensions);
}

TEST(Rect, from_corners)
{
    const Vector2 p0 = {-5.0f, -10.0f};
    const Vector2 p1 = { 3.0f,  11.0f};

    const Rect result = Rect::from_corners(p0, p1);

    ASSERT_EQ(result.origin(), 0.5f * (p0 + p1));
    ASSERT_EQ(result.dimensions(), p1 - p0);
}

TEST(Rect, from_corners_argument_order_doesnt_matter)
{
    const Vector2 p0 = {-5.0f, -10.0f};
    const Vector2 p1 = { 3.0f,  11.0f};

    const Rect result_a = Rect::from_corners(p0, p1);
    const Rect result_b = Rect::from_corners(p1, p0);

    ASSERT_EQ(result_a, result_b);
}

TEST(Rect, dimensions_returns_expected_dimensions)
{
    const Rect rect = Rect::from_corners({-9.0f, 3.0f}, {-13.0f, 9.0f});

    ASSERT_EQ(rect.dimensions(), Vector2(4.0f, 6.0f));
}

TEST(Rect, width_returns_expected_width)
{
    const Rect rect = Rect::from_corners({-9.0f, 3.0f}, {-13.0f, 9.0f});

    ASSERT_EQ(rect.width(), 4.0f);
}

TEST(Rect, height_returns_expected_width)
{
    const Rect rect = Rect::from_corners({-9.0f, 3.0f}, {-13.0f, 9.0f});

    ASSERT_EQ(rect.height(), 6.0f);
}

TEST(Rect, left_returns_expected_left_offset)
{
    const Rect rect = Rect::from_corners({-9.0f, 3.0f}, {-13.0f, 9.0f});

    ASSERT_EQ(rect.left(), -13.0f);
}

TEST(Rect, right_returns_expected_right_offset)
{
    const Rect rect = Rect::from_corners({-9.0f, 3.0f}, {-13.0f, 9.0f});

    ASSERT_EQ(rect.right(), -9.0f);
}

TEST(Rect, ypd_top_returns_expected_top_offset)
{
    const Rect rect = Rect::from_corners({9.0f, 3.0f}, {13.0f, 9.0f});

    ASSERT_EQ(rect.ypd_top(), 3.0f);
}

TEST(Rect, ypd_bottom_returns_expected_bottom_offset)
{
    const Rect rect = Rect::from_corners({9.0f, 3.0f}, {13.0f, 9.0f});

    ASSERT_EQ(rect.ypd_bottom(), 9.0f);
}

TEST(Rect, ypu_top_returns_expected_top_offset)
{
    const Rect rect = Rect::from_corners({9.0f, 3.0f}, {13.0f, 9.0f});

    ASSERT_EQ(rect.ypu_top(), 9.0f);
}

TEST(Rect, ypu_bottom_returns_expected_bottom_offset)
{
    const Rect rect = Rect::from_corners({9.0f, 3.0f}, {13.0f, 9.0f});

    ASSERT_EQ(rect.ypu_bottom(), 3.0f);
}

TEST(Rect, half_extents_returns_expected_half_dimensions)
{
    const Rect rect = Rect::from_corners({9.0f, 3.0f}, {13.0f, 9.0f});

    ASSERT_EQ(rect.half_extents(), Vector2(2.0f, 3.0f));
}

TEST(Rect, area_returns_expected_area)
{
    const Rect rect = Rect::from_corners({5.0f, 3.0f}, {6.0f, 5.0f});

    ASSERT_EQ(rect.area(), 2.0f);
}

TEST(Rect, corner_to_corner_constructor_works_with_min_max_righthanded_corners)
{
    const Rect rect = Rect::from_corners(Vector2{-1.0f}, Vector2{+1.0f});

    ASSERT_EQ(rect.origin(), Vector2{});
    ASSERT_EQ(rect.dimensions(), Vector2{2.0f});
}

TEST(Rect, corners_returns_min_and_max_corners_of_the_rect)
{
    const Rect rect = Rect::from_corners(Vector2{-1.0f}, Vector2{+1.0f});

    const auto corners = rect.corners();

    ASSERT_EQ(corners.min, Vector2{-1.0f});
    ASSERT_EQ(corners.max, Vector2{+1.0f});
}

TEST(Rect, min_corner_returns_expected_result)
{
    const Rect rect = Rect::from_corners(Vector2{-7.0f}, Vector2{+3.0f});

    ASSERT_EQ(rect.min_corner(), Vector2{-7.0f});
}

TEST(Rect, max_corner_returns_expected_result)
{
    const Rect rect = Rect::from_corners(Vector2{-7.0f}, Vector2{+3.0f});

    ASSERT_EQ(rect.max_corner(), Vector2{+3.0f});
}

TEST(Rect, ypd_top_left_returns_expected_point)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    ASSERT_EQ(rect.ypd_top_left(), Vector2(5.0f));
}

TEST(Rect, ypd_top_right_returns_expected_point)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    ASSERT_EQ(rect.ypd_top_right(), Vector2(50.0f, 5.0f));
}

TEST(Rect, ypd_bottom_left_returns_expected_point)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    ASSERT_EQ(rect.ypd_bottom_left(), Vector2(5.0f, 50.0f));
}

TEST(Rect, ypd_bottom_right_returns_expected_point)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    ASSERT_EQ(rect.ypd_bottom_right(), Vector2(50.0f, 50.0f));
}

TEST(Rect, ypu_top_left_returns_expected_point)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    ASSERT_EQ(rect.ypu_top_left(), Vector2(5.0f, 50.0f));
}

TEST(Rect, ypu_top_right_returns_expected_point)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    ASSERT_EQ(rect.ypu_top_right(), Vector2(50.0f));
}

TEST(Rect, ypu_bottom_left_returns_expected_point)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    ASSERT_EQ(rect.ypu_bottom_left(), Vector2(5.0f));
}

TEST(Rect, ypu_bottom_right_returns_expected_point)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    ASSERT_EQ(rect.ypu_bottom_right(), Vector2(50.0f, 5.0f));
}

TEST(Rect, with_dimensions_scaled_by_rescales_dimensions)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    const Rect result = rect.with_dimensions_scaled_by({0.5f, 2.25f});

    ASSERT_EQ(result.dimensions(), Vector2(0.5f*45.0f, 2.25f*45.0f));
}

TEST(Rect, with_dimensions_scaled_by_doesnt_change_origin)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    const Rect result = rect.with_dimensions_scaled_by({0.5f, 2.25f});

    ASSERT_EQ(result.origin(), rect.origin());
}

TEST(Rect, with_origin_and_dimensions_scaled_by_rescales_both_origin_and_dimensions)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    const Rect result = rect.with_origin_and_dimensions_scaled_by(0.5f);

    ASSERT_EQ(result.dimensions(), 0.5f*rect.dimensions());
    ASSERT_EQ(result.origin(), 0.5f*rect.origin());
}

TEST(Rect, with_flipped_y_returns_expected_rect)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    const Rect result = rect.with_flipped_y(125.0f);
    const auto corners = result.corners();

    ASSERT_EQ(corners.min.x, 5.0f);
    ASSERT_EQ(corners.min.y, 75.0f);
    ASSERT_EQ(corners.max.x, 50.0f);
    ASSERT_EQ(corners.max.y, 120.0f);
}

TEST(Rect, with_dimensions_returns_expected_rect)
{
    const Rect rect = Rect::from_corners(Vector2{5.0f}, Vector2{50.0f});

    const Rect result = rect.with_dimensions({1.0f, 1.0f});

    ASSERT_EQ(result.dimensions(), Vector2(1.0f, 1.0f));
}

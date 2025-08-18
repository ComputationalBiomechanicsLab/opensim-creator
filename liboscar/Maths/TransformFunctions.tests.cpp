#include "TransformFunctions.h"

#include <liboscar/Maths/Constants.h>
#include <liboscar/Maths/Transform.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(any_element_is_nan, returns_false_for_default_constructed_Transform)
{
    ASSERT_FALSE(any_element_is_nan(Transform{}));
}

TEST(any_element_is_nan, returns_true_if_scale_x_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.scale = {quiet_nan_v<float>, 1.0f, 1.0f}}));
}

TEST(any_element_is_nan, returns_true_if_scale_y_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.scale = {1.0f, quiet_nan_v<float>, 1.0f}}));
}

TEST(any_element_is_nan, returns_true_if_scale_z_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.scale = {1.0f, 1.0f, quiet_nan_v<float>}}));
}

TEST(any_element_is_nan, returns_true_if_rotation_w_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.rotation = {quiet_nan_v<float>, 0.0f, 0.0f, 0.0f}}));
}

TEST(any_element_is_nan, returns_true_if_rotation_x_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.rotation = {1.0f, quiet_nan_v<float>, 0.0f, 0.0f}}));
}

TEST(any_element_is_nan, returns_true_if_rotation_y_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.rotation = {1.0f, 0.0f, quiet_nan_v<float>, 0.0f}}));
}

TEST(any_element_is_nan, returns_true_if_rotation_z_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.rotation = {1.0f, 0.0f, 0.0f, quiet_nan_v<float>}}));
}

TEST(any_element_is_nan, returns_true_if_position_x_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.translation = {quiet_nan_v<float>, 0.0f, 0.0f}}));
}

TEST(any_element_is_nan, returns_true_if_position_y_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.translation = {0.0f, quiet_nan_v<float>, 0.0f}}));
}

TEST(any_element_is_nan, returns_true_if_position_z_is_nan)
{
    ASSERT_TRUE(any_element_is_nan(Transform{.translation = {0.0f, 0.0f, quiet_nan_v<float>}}));
}

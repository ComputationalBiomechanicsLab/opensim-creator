#include <oscar/Graphics/Scene/SceneHelpers.h>

#include <oscar/Graphics/Scene/SceneCache.h>

#include <gtest/gtest.h>

#include <cstddef>

using namespace osc;

// this exercises relied-upon behavior: downstream code might not check that
// (e.g.) a vector arrow that it wants to draw actually has a length of zero
TEST(draw_arrow, generates_nothing_if_length_between_start_and_end_is_zero)
{
    const ArrowProperties arrow_properties = {
        .start = {1.0f, 0.0f, 0.0f},
        .end = {1.0f, 0.0f, 0.0f},  // uh oh: same location
        .tip_length = 1.0f,
        .neck_thickness = 0.5f,
        .head_thickness = 0.5f,
    };

    SceneCache scene_cache;
    size_t num_decorations_generated = 0;
    draw_arrow(scene_cache, arrow_properties, [&num_decorations_generated](auto&&) { ++num_decorations_generated; });

    ASSERT_EQ(num_decorations_generated, 0);
}

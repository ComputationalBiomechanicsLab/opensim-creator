#include "MathHelpers.h"

#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Vec2.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(ndc_rect_to_topleft_viewport_rect, works_with_basic_example)
{
    const Rect ndc_rect{Vec2{-0.5}, Vec2{+0.5}};
    const Rect viewport_rect{Vec2{10.0f, 100.0f}, Vec2{1034.0f, 1200.0f}};

    const Rect result = ndc_rect_to_topleft_viewport_rect(ndc_rect, viewport_rect);

    ASSERT_EQ(result.origin(), viewport_rect.origin());
    ASSERT_EQ(result.dimensions(), 0.5f * viewport_rect.dimensions());
}

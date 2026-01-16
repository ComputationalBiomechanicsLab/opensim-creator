#include "color_component.h"

#include <liboscar/graphics/unorm8.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(ColorComponent, is_satisfied_by_float)
{
    static_assert(ColorComponent<float>);
}

TEST(ColorComponent, is_satisfied_by_Unorm8)
{
    static_assert(ColorComponent<Unorm8>);
}

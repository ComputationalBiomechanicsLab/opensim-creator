#include <oscar/Graphics/ColorComponent.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Unorm8.h>

using namespace osc;

TEST(ColorComponent, is_satisfied_by_float)
{
    static_assert(ColorComponent<float>);
}

TEST(ColorComponent, is_satisfied_by_Unorm8)
{
    static_assert(ColorComponent<Unorm8>);
}

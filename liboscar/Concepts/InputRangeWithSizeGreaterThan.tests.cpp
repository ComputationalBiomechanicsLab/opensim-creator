#include "InputRangeWithSizeGreaterThan.h"

#include <gtest/gtest.h>

#include <liboscar/Maths/Triangle.h>
#include <liboscar/Maths/Vector3.h>

#include <array>
#include <span>

using namespace osc;

TEST(InputRangeWithSizeGreaterThan, returns_false_for_dynamically_sized_std_span)
{
    static_assert(InputRangeWithSizeGreaterThan<Vector3, 2>);
    static_assert(InputRangeWithSizeGreaterThan<Triangle, 2>);
    static_assert(InputRangeWithSizeGreaterThan<const Triangle&, 2>);
    static_assert(not InputRangeWithSizeGreaterThan<Triangle, 3>);
    static_assert(InputRangeWithSizeGreaterThan<std::span<int, 5>, 4>);
    static_assert(InputRangeWithSizeGreaterThan<std::array<int, 5>, 4>);
}

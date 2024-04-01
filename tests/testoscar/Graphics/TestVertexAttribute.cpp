#include <oscar/Graphics/VertexAttribute.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <algorithm>
#include <array>
#include <tuple>

using namespace osc;

// double-check that the `VertexAttribute` enum is in the same order as data
// will be in the vertex buffer, so that (e.g.) implementations can perform
// set intersection between multiple vertex buffers etc. more easily
TEST(VertexAttribute, OrderMatchesAttributeLayout)
{
    auto const order = std::to_array({
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::Tangent,
        VertexAttribute::Color,
        VertexAttribute::TexCoord0,
    });
    static_assert(std::tuple_size_v<decltype(order)> == num_options<VertexAttribute>());

    ASSERT_TRUE(std::is_sorted(order.begin(), order.end()));
}

#include <oscar/Graphics/VertexAttribute.hpp>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.hpp>

#include <algorithm>
#include <array>
#include <tuple>

using osc::NumOptions;
using osc::VertexAttribute;

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
    static_assert(std::tuple_size_v<decltype(order)> == NumOptions<VertexAttribute>());

    ASSERT_TRUE(std::is_sorted(order.begin(), order.end()));
}

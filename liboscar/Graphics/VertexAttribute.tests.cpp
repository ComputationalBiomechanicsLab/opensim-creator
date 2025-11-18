#include "VertexAttribute.h"

#include <liboscar/Utils/EnumHelpers.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <ranges>
#include <tuple>

using namespace osc;
namespace rgs = std::ranges;

// double-check that the `VertexAttribute` enum class is in the same order as data
// will be in the vertex buffer, so that (e.g.) implementations can perform
// set intersection between multiple vertex buffers etc. more easily
TEST(VertexAttribute, order_matches_default_vertex_buffer_layout)
{
    constexpr auto expected_order = std::to_array({
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::Tangent,
        VertexAttribute::Color,
        VertexAttribute::TexCoord0,
    });
    static_assert(std::tuple_size_v<decltype(expected_order)> == num_options<VertexAttribute>());

    ASSERT_TRUE(rgs::is_sorted(expected_order));
}

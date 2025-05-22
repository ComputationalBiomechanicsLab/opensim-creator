#include "SVG.h"

#include <gtest/gtest.h>

#include <liboscar/Maths/Vec2.h>

#include <sstream>
#include <string>
#include <string_view>

using namespace osc;

namespace
{
    constexpr std::string_view c_minimal_svg = R"(
        <svg height="100" width="100" xmlns="http://www.w3.org/2000/svg">
          <circle r="45" cx="50" cy="50" fill="red" />
        </svg>
    )";
    constexpr Vec2i c_minimal_svg_dimensions = {100, 100};
}

TEST(load_texture2D_from_svg, returns_expected_texture_dimensions_for_basic_case)
{
    std::istringstream ss{std::string{c_minimal_svg}};
    const Texture2D returned_texture = load_texture2D_from_svg(ss);
    ASSERT_EQ(returned_texture.dimensions(), c_minimal_svg_dimensions);
}

TEST(load_texture2D_from_svg, returns_2x_dimension_texture_if_given_2x_scale)
{
    std::istringstream ss{std::string{c_minimal_svg}};
    const Texture2D returned_texture = load_texture2D_from_svg(ss, 2.0f);
    ASSERT_EQ(returned_texture.dimensions(), 2 * c_minimal_svg_dimensions);
}

TEST(load_texture2D_from_svg, throws_if_given_0x_scale)
{
    std::istringstream ss{std::string{c_minimal_svg}};
    ASSERT_ANY_THROW({ load_texture2D_from_svg(ss, 0.0f); });
}

#include "svg.h"

#include <gtest/gtest.h>

#include <liboscar/maths/Vector2.h>

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
    constexpr Vector2i c_minimal_svg_dimensions = {100, 100};
}

TEST(SVG, read_into_texture_returns_expected_texture_dimensions_for_basic_case)
{
    std::istringstream ss{std::string{c_minimal_svg}};
    const Texture2D returned_texture = SVG::read_into_texture(ss);
    ASSERT_EQ(returned_texture.pixel_dimensions(), c_minimal_svg_dimensions);
}

TEST(SVG, read_into_texture_returns_2x_dimension_texture_if_given_2x_scale)
{
    std::istringstream ss{std::string{c_minimal_svg}};
    const Texture2D returned_texture = SVG::read_into_texture(ss, 2.0f);
    ASSERT_EQ(returned_texture.pixel_dimensions(), 2 * c_minimal_svg_dimensions);
}

TEST(SVG, read_into_texture_throws_if_given_0x_scale)
{
    std::istringstream ss{std::string{c_minimal_svg}};
    ASSERT_ANY_THROW({ SVG::read_into_texture(ss, 0.0f); });
}

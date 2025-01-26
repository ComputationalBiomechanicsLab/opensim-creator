#include "TOML.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string_view>

using namespace osc;

TEST(TOML, load_toml_as_flattened_unordered_map_works_on_basic_data)
{
    const std::string input_toml = R"(
        a = 1
        [b]
        c = "hello, world"
    )";

    std::istringstream input{input_toml};
    const auto rv = load_toml_as_flattened_unordered_map(input);

    ASSERT_EQ(rv.size(), 2);
    ASSERT_EQ(rv.at("/a"), Variant{1});
    ASSERT_EQ(rv.at("/b/c"), Variant{"hello, world"});
}

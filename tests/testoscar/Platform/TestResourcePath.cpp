#include <oscar/Platform/ResourcePath.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(ResourcePath, can_default_construct)
{
    [[maybe_unused]] const ResourcePath p{};
}

TEST(ResourcePath, can_construct_from_cstring_literal_filepath)
{
    [[maybe_unused]] const ResourcePath p{"some/path/to/resource.png"};
}

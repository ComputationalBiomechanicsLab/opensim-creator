#include <oscar/Platform/ResourcePath.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(ResourcePath, CanDefaultConstruct)
{
    [[maybe_unused]] ResourcePath p{};
}

TEST(ResourcePath, CanConstructFromStringLiteral)
{
    [[maybe_unused]] ResourcePath p{"some/path/to/resource.png"};
}

#include "resource_directory_entry.h"

#include <liboscar/utilities/string_helpers.h>

#include <gtest/gtest.h>

#include <functional>
#include <string>

using namespace osc;

TEST(ResourceDirectoryEntry, equality_works_as_expected)
{
    ASSERT_EQ(ResourceDirectoryEntry("a.txt", false), ResourceDirectoryEntry("a.txt", false));
    ASSERT_EQ(ResourceDirectoryEntry("a", true), ResourceDirectoryEntry("a", true));
    ASSERT_EQ(ResourceDirectoryEntry("a", false), ResourceDirectoryEntry("a", false));

    ASSERT_NE(ResourceDirectoryEntry("a", true), ResourceDirectoryEntry("b", true));
    ASSERT_NE(ResourceDirectoryEntry("a", true), ResourceDirectoryEntry("a", false));
    ASSERT_NE(ResourceDirectoryEntry("a", true), ResourceDirectoryEntry("b", false));
}

TEST(ResourceDirectoryEntry, is_hashable)
{
    const ResourceDirectoryEntry a1{"a", true};
    const ResourceDirectoryEntry a2{"a", true};
    const ResourceDirectoryEntry b1{"b", true};

    std::hash<ResourceDirectoryEntry> hasher{};

    ASSERT_EQ(hasher(a1), hasher(a2));
    ASSERT_NE(hasher(a1), hasher(b1));
}

TEST(ResourceDirectoryEntry, is_writable_to_stream)
{
    const std::string str = stream_to_string(ResourceDirectoryEntry{"something", true});
    ASSERT_FALSE(str.empty());
}

#include "native_filesystem.h"

#include <gtest/gtest.h>

#include <liboscar/utils/TemporaryDirectory.h>

#include <filesystem>
#include <fstream>
#include <unordered_set>

using namespace osc;

TEST(NativeFilesystem, root_directory_returns_root_directory_path)
{
    const std::filesystem::path root_directory_path = "some/root";
    NativeFilesystem native_filesystem{root_directory_path};

    ASSERT_EQ(native_filesystem.root_directory(), root_directory_path);
}

TEST(NativeFilesystem, resource_exists_returns_false_for_non_existent_path)
{
    NativeFilesystem native_filesystem{"some/root"};

    ASSERT_FALSE(native_filesystem.resource_exists("doesnt-exist"));
}

TEST(NativeFilesystem, resource_exists_returns_true_for_existent_path)
{
    // Setup fixture directory
    TemporaryDirectory root_directory;
    {
        std::ofstream somefile{root_directory.absolute_path() / "somefile"};
    }

    NativeFilesystem native_filesystem{root_directory.absolute_path()};

    ASSERT_TRUE(native_filesystem.resource_exists("somefile"));
}

TEST(NativeFilesystem, resource_exists_returns_false_for_existent_directory)
{
    TemporaryDirectory root_directory;
    std::filesystem::create_directory(root_directory.absolute_path() / "somedir");

    NativeFilesystem native_filesystem{root_directory.absolute_path()};

    ASSERT_FALSE(native_filesystem.resource_exists("somedir"));
}

TEST(NativeFilesystem, open_throws_when_opening_non_existent_path)
{
    NativeFilesystem native_filesystem{"some/root"};

    ASSERT_ANY_THROW({ native_filesystem.open("doesnt-exist"); });
}

TEST(NativeFilesystem, open_throws_when_opening_a_directory)
{
    TemporaryDirectory root_directory;
    std::filesystem::create_directory(root_directory.absolute_path() / "somedir");

    NativeFilesystem native_filesystem{root_directory.absolute_path()};

    ASSERT_ANY_THROW({ native_filesystem.open("somedir"); });
}

TEST(NativeFilesystem, open_returns_ResourceStream_when_opening_a_file)
{
    constexpr std::string_view written_content = "some file content";

    TemporaryDirectory root_directory;
    {
        std::ofstream ofs{root_directory.absolute_path() / "somefile"};
        ofs << written_content;
    }

    NativeFilesystem native_filesystem{root_directory.absolute_path()};

    ResourceStream stream = native_filesystem.open("somefile");
    ASSERT_EQ(stream.name(), "somefile");

    const std::string read_content(std::istreambuf_iterator<char>{stream.stream()}, std::istreambuf_iterator<char>{});

    ASSERT_EQ(read_content, written_content);
}

TEST(NativeFilesystem, iterate_directory_immediately_throws_when_given_non_existent_path)
{
    NativeFilesystem native_filesystem{"some/root"};

    ASSERT_ANY_THROW({ native_filesystem.iterate_directory("doesnt-exist"); });
}

TEST(NativeFilesystem, iterate_directory_immediately_throws_when_given_file_path)
{
    TemporaryDirectory root_directory;
    {
        std::ofstream{root_directory.absolute_path() / "foo"};
    }

    NativeFilesystem native_filesystem{root_directory.absolute_path()};

    ASSERT_ANY_THROW({ native_filesystem.iterate_directory("foo"); });
}

TEST(NativeFilesystem, iterate_directory_yields_paths_relative_to_root_directory)
{
    std::unordered_set<ResourceDirectoryEntry> expected_entries;

    TemporaryDirectory root_directory;
    {
        std::ofstream file1{root_directory.absolute_path() / "file1"};
        expected_entries.emplace("file1", false);
    }

    std::filesystem::create_directory(root_directory.absolute_path() / "dir1");
    expected_entries.emplace("dir1", true);
    {
        // Note: iteration is non-recursive by default.
        std::ofstream file2{root_directory.absolute_path() / "dir1" / "shouldnt-be-visible"};
    }

    NativeFilesystem native_filesystem{root_directory.absolute_path()};
    std::unordered_set<ResourceDirectoryEntry> found_entries;
    for (auto&& entry : native_filesystem.iterate_directory(".")) {
        found_entries.insert(entry);
    }

    ASSERT_EQ(expected_entries, found_entries);
}

TEST(NativeFilesystem, resource_filepath_returns_nullopt_when_given_non_existent_path)
{
    TemporaryDirectory root_directory;
    NativeFilesystem native_filesystem{root_directory.absolute_path()};

    ASSERT_EQ(native_filesystem.resource_filepath("doesnt-exist"), std::nullopt);
}

TEST(NativeFilesystem, resource_filepath_returns_non_nullopt_when_given_an_existent_path)
{
    TemporaryDirectory root_directory;
    {
        std::ofstream{root_directory.absolute_path() / "bar"};
    }
    NativeFilesystem native_filesystem{root_directory.absolute_path()};

    const auto filepath = native_filesystem.resource_filepath("bar");
    ASSERT_TRUE(filepath.has_value());
    ASSERT_EQ(filepath, std::filesystem::weakly_canonical(root_directory.absolute_path() / "bar"));
}

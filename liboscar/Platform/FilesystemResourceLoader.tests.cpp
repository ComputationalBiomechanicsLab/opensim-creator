#include "FilesystemResourceLoader.h"

#include <gtest/gtest.h>

#include <liboscar/Utils/TemporaryDirectory.h>

#include <filesystem>
#include <fstream>
#include <unordered_set>

using namespace osc;

TEST(FilesystemResourceLoader, root_directory_returns_root_directory_path)
{
    const std::filesystem::path root_directory_path = "some/root";
    FilesystemResourceLoader loader{root_directory_path};

    ASSERT_EQ(loader.root_directory(), root_directory_path);
}

TEST(FilesystemResourceLoader, resource_exists_returns_false_for_non_existent_path)
{
    FilesystemResourceLoader loader{"some/root"};

    ASSERT_FALSE(loader.resource_exists("doesnt-exist"));
}

TEST(FilesystemResourceLoader, resource_exists_returns_true_for_existent_path)
{
    // Setup fixture directory
    TemporaryDirectory root_directory;
    {
        std::ofstream somefile{root_directory.absolute_path() / "somefile"};
    }

    FilesystemResourceLoader loader{root_directory.absolute_path()};

    ASSERT_TRUE(loader.resource_exists("somefile"));
}

TEST(FilesystemResourceLoader, resource_exists_returns_true_for_existent_directory)
{
    TemporaryDirectory root_directory;
    std::filesystem::create_directory(root_directory.absolute_path() / "somedir");

    FilesystemResourceLoader loader{root_directory.absolute_path()};

    ASSERT_TRUE(loader.resource_exists("somedir"));
}

TEST(FilesystemResourceLoader, open_throws_when_opening_non_existent_path)
{
    FilesystemResourceLoader loader{"some/root"};

    ASSERT_ANY_THROW({ loader.open("doesnt-exist"); });
}

TEST(FilesystemResourceLoader, open_throws_when_opening_a_directory)
{
    TemporaryDirectory root_directory;
    std::filesystem::create_directory(root_directory.absolute_path() / "somedir");

    FilesystemResourceLoader loader{root_directory.absolute_path()};

    ASSERT_ANY_THROW({ loader.open("somedir"); });
}

TEST(FilesystemResourceLoader, open_returns_ResourceStream_when_opening_a_file)
{
    constexpr std::string_view written_content = "some file content";

    TemporaryDirectory root_directory;
    {
        std::ofstream ofs{root_directory.absolute_path() / "somefile"};
        ofs << written_content;
    }

    FilesystemResourceLoader loader{root_directory.absolute_path()};

    ResourceStream stream = loader.open("somefile");
    ASSERT_EQ(stream.name(), "somefile");

    const std::string read_content(std::istreambuf_iterator<char>{stream.stream()}, std::istreambuf_iterator<char>{});

    ASSERT_EQ(read_content, written_content);
}

TEST(FilesystemResourceLoader, iterate_directory_immediately_throws_when_given_non_existent_path)
{
    FilesystemResourceLoader loader{"some/root"};

    ASSERT_ANY_THROW({ loader.iterate_directory("doesnt-exist"); });
}

TEST(FilesystemResourceLoader, iterate_directory_immediately_throws_when_given_file_path)
{
    TemporaryDirectory root_directory;
    {
        std::ofstream{root_directory.absolute_path() / "foo"};
    }

    FilesystemResourceLoader loader{root_directory.absolute_path()};

    ASSERT_ANY_THROW({ loader.iterate_directory("foo"); });
}

TEST(FilesystemResourceLoader, iterate_directory_yields_paths_relative_to_root_directory)
{
    std::unordered_set<ResourceDirectoryEntry> expected_entries;

    TemporaryDirectory root_directory;
    {
        std::ofstream{root_directory.absolute_path() / "file1"};
        expected_entries.emplace("file1", false);
    }

    std::filesystem::create_directory(root_directory.absolute_path() / "dir1");
    expected_entries.emplace("dir1", true);
    {
        // Note: iteration is non-recursive by default.
        std::ofstream{root_directory.absolute_path() / "dir1" / "shouldnt-be-visible"};
    }

    FilesystemResourceLoader loader{root_directory.absolute_path()};
    std::unordered_set<ResourceDirectoryEntry> found_entries;
    for (auto&& entry : loader.iterate_directory(".")) {
        found_entries.insert(entry);
    }

    ASSERT_EQ(expected_entries, found_entries);
}

TEST(FilesystemResourceLoader, resource_filepath_returns_nullopt_when_given_non_existent_path)
{
    TemporaryDirectory root_directory;
    FilesystemResourceLoader loader{root_directory.absolute_path()};

    ASSERT_EQ(loader.resource_filepath("doesnt-exist"), std::nullopt);
}

TEST(FilesystemResourceLoader, resource_filepath_returns_non_nullopt_when_given_an_existent_path)
{
    TemporaryDirectory root_directory;
    {
        std::ofstream{root_directory.absolute_path() / "bar"};
    }
    FilesystemResourceLoader loader{root_directory.absolute_path()};
    
    const auto filepath = loader.resource_filepath("bar");
    ASSERT_TRUE(filepath.has_value());
    ASSERT_EQ(filepath, root_directory.absolute_path() / "bar");
}

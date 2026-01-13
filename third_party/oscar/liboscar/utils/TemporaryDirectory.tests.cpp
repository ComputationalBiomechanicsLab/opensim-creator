#include "TemporaryDirectory.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ranges>

using namespace osc;

TEST(TemporaryDirectory, can_default_construct)
{
    [[maybe_unused]] const TemporaryDirectory temporary_directory;
}

TEST(TemporaryDirectory, default_constructed_exists_on_filesystem_as_an_empty_directory)
{
    const TemporaryDirectory temporary_directory;
    ASSERT_TRUE(std::filesystem::exists(temporary_directory.absolute_path()));
    ASSERT_TRUE(std::filesystem::is_directory(temporary_directory.absolute_path()));

    std::filesystem::recursive_directory_iterator it{temporary_directory.absolute_path()};
    ASSERT_EQ(std::distance(std::filesystem::begin(it), std::filesystem::end(it)), 0) << "The directory should be empty";
}

TEST(TemporaryDirectory, is_removed_by_destructor)
{
    std::filesystem::path directory_path;
    {
        const TemporaryDirectory temporary_directory;
        directory_path = temporary_directory.absolute_path();

        ASSERT_TRUE(std::filesystem::exists(directory_path));
        ASSERT_TRUE(std::filesystem::is_directory(directory_path));
    }
    ASSERT_FALSE(std::filesystem::exists(directory_path));
}

TEST(TemporaryDirectory, non_empty_directories_also_removed_by_destructor)
{
    std::filesystem::path directory_path;
    std::filesystem::path subdir_path;
    std::filesystem::path subdir_file_path;
    std::filesystem::path subfile_path;
    {
        const TemporaryDirectory temporary_directory;
        directory_path = temporary_directory.absolute_path();

        subdir_path = directory_path / "subdir";
        std::filesystem::create_directory(subdir_path);
        {
            subdir_file_path = subdir_path / "subsubfile";
            std::ofstream{subdir_file_path};  // Creates file
        }
        {
            subfile_path = directory_path / "subfile";
            std::ofstream{subfile_path};  // Creates file
        }

        ASSERT_TRUE(std::filesystem::exists(directory_path));
        ASSERT_TRUE(std::filesystem::is_directory(directory_path));
        ASSERT_TRUE(std::filesystem::exists(subdir_path));
        ASSERT_TRUE(std::filesystem::is_directory(subdir_path));
        ASSERT_TRUE(std::filesystem::exists(subdir_file_path));
        ASSERT_TRUE(std::filesystem::is_regular_file(subdir_file_path));
        ASSERT_TRUE(std::filesystem::exists(subfile_path));
        ASSERT_TRUE(std::filesystem::is_regular_file(subfile_path));
    }

    ASSERT_FALSE(std::filesystem::exists(directory_path));
    ASSERT_FALSE(std::filesystem::exists(subdir_path));
    ASSERT_FALSE(std::filesystem::exists(subdir_file_path));
    ASSERT_FALSE(std::filesystem::exists(subfile_path));
}

TEST(TemporaryDirectory, filename_begins_with_prefix_when_constructed_with_a_prefix)
{
    const TemporaryDirectory temporary_directory({ .prefix = "someprefix" });
    ASSERT_TRUE(temporary_directory.filename().string().starts_with("someprefix"));
}

TEST(TemporaryDirectory, filename_ends_with_suffix_when_constructed_with_a_suffix)
{
    const TemporaryDirectory temporary_directory({ .suffix = "somesuffix" });
    ASSERT_TRUE(temporary_directory.filename().string().ends_with("somesuffix"));
}

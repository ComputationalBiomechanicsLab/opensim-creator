#include <oscar/Utils/TemporaryFile.h>

#include <gtest/gtest.h>
#include <oscar/Utils/StringHelpers.h>

#include <filesystem>

using namespace osc;

TEST(TemporaryFile, can_default_construct)
{
    [[maybe_unused]] TemporaryFile temporary_file;
}

TEST(TemporaryFile, stream_is_open_returns_true_on_default_construction)
{
    TemporaryFile temporary_file;
    ASSERT_TRUE(temporary_file.stream().is_open());
}

TEST(TemporaryFile, file_exists_on_filesystem_after_default_construction)
{
    TemporaryFile temporary_file;
    ASSERT_TRUE(std::filesystem::exists(temporary_file.absolute_path()));
}

TEST(TemporaryFile, file_stops_existing_once_temporary_file_drops_out_of_scope)
{
    std::filesystem::path abs_path;
    {
        TemporaryFile temporary_file;
        abs_path = temporary_file.absolute_path();
        ASSERT_TRUE(std::filesystem::exists(abs_path));
    }
    ASSERT_FALSE(std::filesystem::exists(abs_path));
}

TEST(TemporaryFile, file_name_begins_with_prefix_when_constructed_with_a_prefix)
{
    TemporaryFile temporary_file({ .prefix = "someprefix" });
    ASSERT_TRUE(temporary_file.file_name().string().starts_with("someprefix"));
}

TEST(TemporaryFile, file_name_ends_with_suffix_when_constructed_with_a_suffix)
{
    TemporaryFile temporary_file({ .suffix = "somesuffix" });
    ASSERT_TRUE(temporary_file.file_name().string().ends_with("somesuffix"));
}

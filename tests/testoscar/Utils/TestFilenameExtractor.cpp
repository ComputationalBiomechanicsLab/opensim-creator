#include <oscar/Utils/FilenameExtractor.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(extract_filename, returns_blank_when_called_with_blank_string)
{
    static_assert(extract_filename("").empty());
}

TEST(extract_filename, works_for_unix_paths)
{
    static_assert(extract_filename("/home/user/file.cpp") == "file.cpp");
}

TEST(extract_filename, works_for_Windows_paths)
{
    static_assert(extract_filename(R"(C:\Users\user\file.cpp)") == "file.cpp");
}

TEST(extract_filename, works_for_mixed_path)
{
    // https://stackoverflow.com/a/8488201 (comments section)
    static_assert(extract_filename("C:\\Users\\user/file.cpp") == "file.cpp");
}

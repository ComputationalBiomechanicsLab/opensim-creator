#include <oscar/Utils/FilenameExtractor.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(extract_filename, ReturnsBlankForBlankString)
{
    static_assert(extract_filename("").empty());
}

TEST(FilenameExtractor, WorksAsIntendedForUnixPaths)
{
    static_assert(extract_filename("/home/user/file.cpp") == "file.cpp");
}

TEST(FilenameExtractor, WorksAsIntendedForWindowsPaths)
{
    static_assert(extract_filename(R"(C:\Users\user\file.cpp)") == "file.cpp");
}

TEST(FilenameExtractor, WorksForMixedPath)
{
    // https://stackoverflow.com/a/8488201 (comments section)
    static_assert(extract_filename("C:\\Users\\user/file.cpp") == "file.cpp");
}

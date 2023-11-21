#include <oscar/Utils/FilenameExtractor.hpp>

#include <gtest/gtest.h>

TEST(ExtractFilename, ReturnsBlankForBlankString)
{
    static_assert(osc::ExtractFilename("").empty());
}

TEST(FilenameExtractor, WorksAsIntendedForUnixPaths)
{
    static_assert(osc::ExtractFilename("/home/user/file.cpp") == "file.cpp");
}

TEST(FilenameExtractor, WorksAsIntendedForWindowsPaths)
{
    static_assert(osc::ExtractFilename(R"(C:\Users\user\file.cpp)") == "file.cpp");
}

TEST(FilenameExtractor, WorksForMixedPath)
{
    // https://stackoverflow.com/a/8488201 (comments section)
    static_assert(osc::ExtractFilename("C:\\Users\\user/file.cpp") == "file.cpp");
}

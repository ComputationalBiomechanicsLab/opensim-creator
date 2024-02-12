#include <oscar/Utils/FilenameExtractor.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(ExtractFilename, ReturnsBlankForBlankString)
{
    static_assert(ExtractFilename("").empty());
}

TEST(FilenameExtractor, WorksAsIntendedForUnixPaths)
{
    static_assert(ExtractFilename("/home/user/file.cpp") == "file.cpp");
}

TEST(FilenameExtractor, WorksAsIntendedForWindowsPaths)
{
    static_assert(ExtractFilename(R"(C:\Users\user\file.cpp)") == "file.cpp");
}

TEST(FilenameExtractor, WorksForMixedPath)
{
    // https://stackoverflow.com/a/8488201 (comments section)
    static_assert(ExtractFilename("C:\\Users\\user/file.cpp") == "file.cpp");
}

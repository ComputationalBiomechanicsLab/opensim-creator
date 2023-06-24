#pragma once

#include <nonstd/span.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // recursively find all files in `root` with any of the given extensions and
    // return those files in a vector
    std::vector<std::filesystem::path> FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        nonstd::span<std::string_view> extensions
    );
    std::vector<std::filesystem::path> FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        std::string_view extension
    );

    // recursively find all files in the supplied (root) directory and return
    // them in a vector
    std::vector<std::filesystem::path> GetAllFilesInDirRecursively(std::filesystem::path const&);

    // slurp a file's contents into a string
    std::string SlurpFileIntoString(std::filesystem::path const&);

    // slurp a file's contents into a vector
    std::vector<uint8_t> SlurpFileIntoVector(std::filesystem::path const&);

    // returns the given path's filename without an extension (e.g. /dir/model.osim --> model)
    std::string FileNameWithoutExtension(std::filesystem::path const&);

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2);

    // returns true if `path` is within `dir` (non-recursive)
    bool IsSubpath(std::filesystem::path const& dir, std::filesystem::path const& path);
}

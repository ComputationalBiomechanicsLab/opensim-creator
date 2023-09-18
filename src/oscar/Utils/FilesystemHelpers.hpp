#pragma once

#include <nonstd/span.hpp>

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // calls `consumer` with each file recursively found in `root` that ends with
    // any of the provied `extensions`
    void ForEachFileWithExtensionsRecursive(
        std::filesystem::path const& root,
        std::function<void(std::filesystem::path)> const& consumer,
        nonstd::span<std::string_view const> extensions
    );

    // returns all files found recursively in `root` that end with any of the provided
    // `extensions`
    std::vector<std::filesystem::path> FindFilesWithExtensionsRecursive(
        std::filesystem::path const& root,
        nonstd::span<std::string_view const> extensions
    );

    // calls `consumer` with each file recursively found in `root`
    void ForEachFileRecursive(
        std::filesystem::path const& root,
        std::function<void(std::filesystem::path)> const& consumer
    );

    // returns all files found recursively in `root`
    std::vector<std::filesystem::path> FindFilesRecursive(
        std::filesystem::path const& root
    );

    // slurp a file's contents into a string
    std::string SlurpFileIntoString(std::filesystem::path const&);

    // returns the given path's filename without an extension (e.g. /dir/model.osim --> model)
    std::string FileNameWithoutExtension(std::filesystem::path const&);

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2);

    // returns true if `path` is within `dir` (non-recursive)
    bool IsSubpath(std::filesystem::path const& dir, std::filesystem::path const& path);
}

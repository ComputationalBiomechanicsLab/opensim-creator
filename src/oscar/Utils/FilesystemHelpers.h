#pragma once

#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // calls `consumer` with each file recursively found in `root` that ends with
    // any of the provied `extensions`
    void ForEachFileWithExtensionsRecursive(
        const std::filesystem::path& root,
        const std::function<void(std::filesystem::path)>& consumer,
        std::span<const std::string_view> extensions
    );

    // returns all files found recursively in `root` that end with any of the provided
    // `extensions`
    std::vector<std::filesystem::path> FindFilesWithExtensionsRecursive(
        const std::filesystem::path& root,
        std::span<const std::string_view> extensions
    );

    // calls `consumer` with each file recursively found in `root`
    void ForEachFileRecursive(
        const std::filesystem::path& root,
        const std::function<void(std::filesystem::path)>& consumer
    );

    // returns all files found recursively in `root`
    std::vector<std::filesystem::path> FindFilesRecursive(
        const std::filesystem::path& root
    );

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool IsFilenameLexographicallyGreaterThan(const std::filesystem::path& p1, const std::filesystem::path& p2);

    // returns true if `path` is within `dir` (non-recursive)
    bool IsSubpath(const std::filesystem::path& dir, const std::filesystem::path& path);
}

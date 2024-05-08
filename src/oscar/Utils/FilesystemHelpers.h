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
    void for_each_file_with_extensions_recursive(
        const std::filesystem::path& root,
        const std::function<void(std::filesystem::path)>& consumer,
        std::span<const std::string_view> extensions
    );

    // returns all files found recursively in `root` that end with any of the provided
    // `extensions`
    std::vector<std::filesystem::path> find_files_with_extensions_recursive(
        const std::filesystem::path& root,
        std::span<const std::string_view> extensions
    );

    // calls `consumer` with each file recursively found in `root`
    void for_each_file_recursive(
        const std::filesystem::path& root,
        const std::function<void(std::filesystem::path)>& consumer
    );

    // returns all files found recursively in `root`
    std::vector<std::filesystem::path> find_files_recursive(
        const std::filesystem::path& root
    );

    // returns true if `p1` is lexographically greater than `p2`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool is_filename_lexographically_greater_than(const std::filesystem::path& p1, const std::filesystem::path& p2);

    // returns true if `path` is within `dir` (non-recursive)
    bool is_subpath(const std::filesystem::path& dir, const std::filesystem::path& path);
}

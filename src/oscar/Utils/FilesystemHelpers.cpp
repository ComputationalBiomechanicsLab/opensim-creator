#include "FilesystemHelpers.h"

#include <oscar/Platform/os.h>
#include <oscar/Utils/StringHelpers.h>

#include <algorithm>
#include <string>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <sstream>
#include <utility>

void osc::for_each_file_with_extensions_recursive(
    const std::filesystem::path& root,
    const std::function<void(std::filesystem::path)>& consumer,
    std::span<const std::string_view> extensions)
{
    if (not std::filesystem::exists(root)) {
        return;
    }

    if (not std::filesystem::is_directory(root)) {
        return;
    }

    for (const auto& directory_entry : std::filesystem::recursive_directory_iterator{root}) {

        if (directory_entry.is_regular_file() or directory_entry.is_block_file()) {
            for (const std::string_view& extension : extensions) {
                if (directory_entry.path().extension() == extension) {
                    consumer(directory_entry.path());
                }
            }
        }
    }
}

std::vector<std::filesystem::path> osc::find_files_with_extensions_recursive(
    const std::filesystem::path& root,
    std::span<const std::string_view> extensions)
{
    std::vector<std::filesystem::path> rv;
    for_each_file_with_extensions_recursive(
        root,
        [&rv](auto p) { rv.push_back(std::move(p)); },
        extensions
    );
    return rv;
}

void osc::for_each_file_recursive(
    const std::filesystem::path& root,
    const std::function<void(std::filesystem::path)>& consumer)
{
    if (not std::filesystem::exists(root)) {
        return;
    }

    if (not std::filesystem::is_directory(root)) {
        return;
    }

    for (const auto& directory_entry : std::filesystem::recursive_directory_iterator{root}) {
        if (directory_entry.is_regular_file() or directory_entry.is_block_file()) {
            consumer(directory_entry.path());
        }
    }
}

std::vector<std::filesystem::path> osc::find_files_recursive(
    const std::filesystem::path& root)
{
    std::vector<std::filesystem::path> rv;
    for_each_file_recursive(root, [&rv](auto p) { rv.push_back(std::move(p)); });
    return rv;
}

bool osc::is_filename_lexographically_greater_than(const std::filesystem::path& p1, const std::filesystem::path& p2)
{
    return is_string_case_insensitive_greater_than(p1.filename().string(), p2.filename().string());
}

bool osc::is_subpath(const std::filesystem::path& dir, const std::filesystem::path& path)
{
    auto num_dir_components = std::distance(dir.begin(), dir.end());
    auto num_path_components = std::distance(path.begin(), path.end());

    if (num_path_components < num_dir_components) {
        return false;
    }

    return std::equal(dir.begin(), dir.end(), path.begin());
}

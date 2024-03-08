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

void osc::ForEachFileWithExtensionsRecursive(
    std::filesystem::path const& root,
    std::function<void(std::filesystem::path)> const& consumer,
    std::span<std::string_view const> extensions)
{
    if (!std::filesystem::exists(root))
    {
        return;
    }

    if (!std::filesystem::is_directory(root))
    {
        return;
    }

    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{root})
    {
        if (e.is_regular_file() || e.is_block_file())
        {
            for (std::string_view const& extension : extensions)
            {
                if (e.path().extension() == extension)
                {
                    consumer(e.path());
                }
            }
        }
    }
}

std::vector<std::filesystem::path> osc::FindFilesWithExtensionsRecursive(
    std::filesystem::path const& root,
    std::span<std::string_view const> extensions)
{
    std::vector<std::filesystem::path> rv;
    ForEachFileWithExtensionsRecursive(
        root,
        [&rv](auto p) { rv.push_back(std::move(p)); },
        extensions
    );
    return rv;
}

void osc::ForEachFileRecursive(
    std::filesystem::path const& root,
    std::function<void(std::filesystem::path)> const& consumer)
{
    if (!std::filesystem::exists(root))
    {
        return;
    }

    if (!std::filesystem::is_directory(root))
    {
        return;
    }

    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{root})
    {
        if (e.is_regular_file() || e.is_block_file())
        {
            consumer(e.path());
        }
    }
}

std::vector<std::filesystem::path> osc::FindFilesRecursive(
    std::filesystem::path const& root)
{
    std::vector<std::filesystem::path> rv;
    ForEachFileRecursive(root, [&rv](auto p) { rv.push_back(std::move(p)); });
    return rv;
}

bool osc::IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2)
{
    return IsStringCaseInsensitiveGreaterThan(p1.filename().string(), p2.filename().string());
}

bool osc::IsSubpath(std::filesystem::path const& dir, std::filesystem::path const& path)
{
    auto dirNumComponents = std::distance(dir.begin(), dir.end());
    auto pathNumComponents = std::distance(path.begin(), path.end());

    if (pathNumComponents < dirNumComponents)
    {
        return false;
    }

    return std::equal(dir.begin(), dir.end(), path.begin());
}

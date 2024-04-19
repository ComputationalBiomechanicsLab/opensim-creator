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
    const std::filesystem::path& root,
    const std::function<void(std::filesystem::path)>& consumer,
    std::span<const std::string_view> extensions)
{
    if (!std::filesystem::exists(root)) {
        return;
    }

    if (!std::filesystem::is_directory(root)) {
        return;
    }

    for (const std::filesystem::directory_entry& e : std::filesystem::recursive_directory_iterator{root})
    {
        if (e.is_regular_file() || e.is_block_file())
        {
            for (const std::string_view& extension : extensions)
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
    const std::filesystem::path& root,
    std::span<const std::string_view> extensions)
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
    const std::filesystem::path& root,
    const std::function<void(std::filesystem::path)>& consumer)
{
    if (!std::filesystem::exists(root))
    {
        return;
    }

    if (!std::filesystem::is_directory(root))
    {
        return;
    }

    for (const std::filesystem::directory_entry& e : std::filesystem::recursive_directory_iterator{root})
    {
        if (e.is_regular_file() || e.is_block_file())
        {
            consumer(e.path());
        }
    }
}

std::vector<std::filesystem::path> osc::FindFilesRecursive(
    const std::filesystem::path& root)
{
    std::vector<std::filesystem::path> rv;
    ForEachFileRecursive(root, [&rv](auto p) { rv.push_back(std::move(p)); });
    return rv;
}

bool osc::IsFilenameLexographicallyGreaterThan(const std::filesystem::path& p1, const std::filesystem::path& p2)
{
    return IsStringCaseInsensitiveGreaterThan(p1.filename().string(), p2.filename().string());
}

bool osc::IsSubpath(const std::filesystem::path& dir, const std::filesystem::path& path)
{
    auto dirNumComponents = std::distance(dir.begin(), dir.end());
    auto pathNumComponents = std::distance(path.begin(), path.end());

    if (pathNumComponents < dirNumComponents)
    {
        return false;
    }

    return std::equal(dir.begin(), dir.end(), path.begin());
}

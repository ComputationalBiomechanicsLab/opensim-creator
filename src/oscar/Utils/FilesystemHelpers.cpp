#include "FilesystemHelpers.hpp"

#include <oscar/Platform/os.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <nonstd/span.hpp>

#include <string>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

void osc::ForEachFileWithExtensionsRecursive(
    std::filesystem::path const& root,
    std::function<void(std::filesystem::path)> const& consumer,
    nonstd::span<std::string_view const> extensions)
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
    nonstd::span<std::string_view const> extensions)
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

std::string osc::SlurpFileIntoString(std::filesystem::path const& p)
{
    std::ifstream f{p, std::ios::binary | std::ios::in};

    if (!f)
    {
        std::stringstream msg;
        msg << p << ": error opening file: " << osc::CurrentErrnoAsString();
        throw std::runtime_error{std::move(msg).str()};
    }

    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::stringstream ss;
    try
    {
        ss << f.rdbuf();
    }
    catch (std::exception const& ex)
    {
        std::stringstream msg;
        msg << p << ": error reading file: " << ex.what() << "(strerror = " << osc::CurrentErrnoAsString() << ')';
        throw std::runtime_error{std::move(msg).str()};
    }

    return std::move(ss).str();
}

std::string osc::FileNameWithoutExtension(std::filesystem::path const& p)
{
    return p.filename().replace_extension({}).string();
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

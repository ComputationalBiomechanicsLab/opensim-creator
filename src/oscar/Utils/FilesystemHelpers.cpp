#include "FilesystemHelpers.hpp"

#include "oscar/Platform/os.hpp"
#include "oscar/Utils/StringHelpers.hpp"

#include <nonstd/span.hpp>

#include <cerrno>
#include <cstring>
#include <string>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

std::vector<std::filesystem::path> osc::FindAllFilesWithExtensionsRecursively(
    std::filesystem::path const& root,
    nonstd::span<std::string_view> extensions)
{
    std::vector<std::filesystem::path> rv;

    if (!std::filesystem::exists(root))
    {
        return rv;
    }

    if (!std::filesystem::is_directory(root))
    {
        return rv;
    }

    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{root})
    {
        for (std::string_view const& extension : extensions)
        {
            if (e.path().extension() == extension)
            {
                rv.push_back(e.path());
            }
        }
    }

    return rv;
}

std::vector<std::filesystem::path> osc::FindAllFilesWithExtensionsRecursively(
    std::filesystem::path const& root,
    std::string_view extension)
{
    return FindAllFilesWithExtensionsRecursively(root, {&extension, 1});
}

std::vector<std::filesystem::path> osc::GetAllFilesInDirRecursively(std::filesystem::path const& root)
{
    std::vector<std::filesystem::path> rv;

    if (!std::filesystem::exists(root))
    {
        return rv;
    }

    if (!std::filesystem::is_directory(root))
    {
        return rv;
    }

    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{root})
    {
        if (e.is_directory() || e.is_other() || e.is_socket())
        {
            continue;
        }

        rv.push_back(e.path());
    }

    return rv;
}

std::string osc::SlurpFileIntoString(std::filesystem::path const& p)
{
    std::ifstream f{p, std::ios::binary | std::ios::in};

    if (!f)
    {
        std::stringstream msg;
        msg << p << ": error opening file: " << osc::StrerrorThreadsafe(errno);
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
        msg << p << ": error reading file: " << ex.what() << "(strerror = " << osc::StrerrorThreadsafe(errno) << ')';
        throw std::runtime_error{std::move(msg).str()};
    }

    return std::move(ss).str();
}

std::vector<uint8_t> osc::SlurpFileIntoVector(std::filesystem::path const& p)
{
    std::ifstream f{p, std::ios::binary | std::ios::in};

    if (!f)
    {
        std::stringstream msg;
        msg << p << ": error opening file: " << osc::StrerrorThreadsafe(errno);
        throw std::runtime_error{std::move(msg).str()};
    }
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::streampos const start = f.tellg();
    f.seekg(0, std::ios::end);
    std::streampos const end = f.tellg();
    f.seekg(0, std::ios::beg);

    std::vector<uint8_t> rv;
    rv.reserve(static_cast<size_t>(end - start));

    rv.insert(
        rv.begin(),
        std::istreambuf_iterator<char>{f},
        std::istreambuf_iterator<char>{}
    );

    return rv;
}

std::string osc::FileNameWithoutExtension(std::filesystem::path const& p)
{
    return p.filename().replace_extension("").string();
}

bool osc::IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2)
{
    return IsStringCaseInsensitiveGreaterThan(p1.filename().string(), p2.filename().string());
}

bool osc::IsSubpath(std::filesystem::path const& dir, std::filesystem::path const& pth)
{
    auto dirNumComponents = std::distance(dir.begin(), dir.end());
    auto pathNumComponents = std::distance(pth.begin(), pth.end());

    if (pathNumComponents < dirNumComponents)
    {
        return false;
    }

    return std::equal(dir.begin(), dir.end(), pth.begin());
}
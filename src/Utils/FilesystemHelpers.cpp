#include "FilesystemHelpers.hpp"

#include <cerrno>
#include <cstring>
#include <string>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

void osc::FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n,
        std::vector<std::filesystem::path>& appendOut)
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
        for (size_t i = 0; i < n; ++i)
        {
            if (e.path().extension() == extensions[i])
            {
                appendOut.push_back(e.path());
            }
        }
    }
}

std::vector<std::filesystem::path> osc::FindAllFilesWithExtensionsRecursively(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n)
{
    std::vector<std::filesystem::path> rv;
    FindAllFilesWithExtensionsRecursively(root, extensions, n, rv);
    return rv;
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
        msg << p << ": error opening file: " << strerror(errno);
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
        msg << p << ": error reading file: " << ex.what() << "(strerror = " << strerror(errno) << ')';
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
        msg << p << ": error opening file: " << strerror(errno);
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

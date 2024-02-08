#include "FilesystemResourceLoader.hpp"

#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/ResourcePath.hpp>
#include <oscar/Platform/ResourceStream.hpp>

#include <filesystem>
#include <functional>

using namespace osc;

namespace
{
    std::filesystem::path CalcFullPath(std::filesystem::path const& root, ResourcePath const& subpath)
    {
        return std::filesystem::weakly_canonical(root / subpath.string());
    }
}

ResourceStream osc::FilesystemResourceLoader::implOpen(ResourcePath const& p)
{
    if (log_level() <= LogLevel::debug) {
        log_debug("opening %s", p.string().c_str());
    }
    return ResourceStream{CalcFullPath(m_Root, p)};
}

std::function<std::optional<ResourceDirectoryEntry>()> osc::FilesystemResourceLoader::implIterateDirectory(ResourcePath const& p)
{
    using std::begin;
    using std::end;

    std::filesystem::path dirRoot = CalcFullPath(m_Root, p);
    std::filesystem::directory_iterator iterable{dirRoot};
    return [p, dirRoot, beg = begin(iterable), en = end(iterable)]() mutable -> std::optional<ResourceDirectoryEntry>
    {
        if (beg != en) {
            auto relpath = std::filesystem::relative(beg->path(), dirRoot);
            ResourceDirectoryEntry rv{relpath.string(), beg->is_directory()};
            ++beg;
            return rv;
        } else {
            return std::nullopt;
        }
    };
}

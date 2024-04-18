#include "FilesystemResourceLoader.h"

#include <oscar/Platform/Log.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Platform/ResourceStream.h>

#include <filesystem>
#include <functional>

using namespace osc;

namespace
{
    std::filesystem::path calc_full_path(const std::filesystem::path& root, const ResourcePath& subpath)
    {
        return std::filesystem::weakly_canonical(root / subpath.string());
    }
}

ResourceStream osc::FilesystemResourceLoader::impl_open(const ResourcePath& p)
{
    if (log_level() <= LogLevel::debug) {
        log_debug("opening %s", p.string().c_str());
    }
    return ResourceStream{calc_full_path(root_directory_, p)};
}

std::function<std::optional<ResourceDirectoryEntry>()> osc::FilesystemResourceLoader::impl_iterate_directory(const ResourcePath& p)
{
    using std::begin;
    using std::end;

    const std::filesystem::path full_path = calc_full_path(root_directory_, p);
    const std::filesystem::directory_iterator iterable{full_path};
    return [p, full_path, beg = begin(iterable), en = end(iterable)]() mutable -> std::optional<ResourceDirectoryEntry>
    {
        if (beg != en) {
            const auto relative_path = std::filesystem::relative(beg->path(), full_path);
            ResourceDirectoryEntry rv{relative_path.string(), beg->is_directory()};
            ++beg;
            return rv;
        } else {
            return std::nullopt;
        }
    };
}

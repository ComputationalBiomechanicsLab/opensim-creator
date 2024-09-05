#include "FilesystemResourceLoader.h"

#include <oscar/Platform/Log.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Platform/ResourceStream.h>

#include <filesystem>
#include <functional>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    std::filesystem::path calc_full_path(const std::filesystem::path& root, const ResourcePath& subpath)
    {
        return std::filesystem::weakly_canonical(root / subpath.string());
    }
}

ResourceStream osc::FilesystemResourceLoader::impl_open(const ResourcePath& resource_path)
{
    if (log_level() <= LogLevel::debug) {
        log_debug("opening %s", resource_path.string().c_str());
    }
    return ResourceStream{calc_full_path(root_directory_, resource_path)};
}

std::function<std::optional<ResourceDirectoryEntry>()> osc::FilesystemResourceLoader::impl_iterate_directory(const ResourcePath& resource_path)
{
    const std::filesystem::path full_path = calc_full_path(root_directory_, resource_path);
    const std::filesystem::directory_iterator iterable{full_path};
    return [resource_path, full_path, beg = rgs::begin(iterable), en = rgs::end(iterable)]() mutable -> std::optional<ResourceDirectoryEntry>
    {
        if (beg != en) {
            const auto relative_path = std::filesystem::relative(beg->path(), full_path);
            ResourceDirectoryEntry rv{relative_path.string(), beg->is_directory()};
            ++beg;
            return rv;
        }
        else {
            return std::nullopt;
        }
    };
}

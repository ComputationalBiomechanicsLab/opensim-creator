#include "native_filesystem.h"

#include <liboscar/platform/log.h>
#include <liboscar/platform/resource_path.h>
#include <liboscar/platform/resource_stream.h>
#include <liboscar/shims/cpp23/generator.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    std::filesystem::path calc_full_path(const std::filesystem::path& root, const ResourcePath& subpath)
    {
        return std::filesystem::weakly_canonical(root / subpath.string());
    }

    cpp23::generator<ResourceDirectoryEntry> iterate_directory_async(std::filesystem::path full_path)
    {
        const std::filesystem::directory_iterator iterable{full_path};
        for (auto it = rgs::begin(iterable), end = rgs::end(iterable); it != end; ++it) {
            const auto relative_path = std::filesystem::relative(it->path(), full_path);
            co_yield ResourceDirectoryEntry{relative_path.string(), it->is_directory()};
        }
    }
}

std::optional<std::filesystem::path> osc::NativeFilesystem::resource_filepath(const ResourcePath& resource_path) const
{
    std::filesystem::path full_path = calc_full_path(root_directory_, resource_path);
    return std::filesystem::exists(full_path) ? std::optional{std::move(full_path)} : std::nullopt;
}

bool osc::NativeFilesystem::impl_resource_exists(const ResourcePath& resource_path)
{
    const std::filesystem::path full_path = calc_full_path(root_directory_, resource_path);
    std::error_code ec;
    const std::filesystem::file_status status = std::filesystem::status(full_path, ec);
    return (not ec) and (not std::filesystem::is_directory(status));
}

ResourceStream osc::NativeFilesystem::impl_open(const ResourcePath& resource_path)
{
    if (log_level() <= LogLevel::debug) {
        log_debug("opening %s", resource_path.string().c_str());
    }
    return ResourceStream{calc_full_path(root_directory_, resource_path)};
}

cpp23::generator<ResourceDirectoryEntry> osc::NativeFilesystem::impl_iterate_directory(ResourcePath resource_path)
{
    std::filesystem::path full_path = calc_full_path(root_directory_, resource_path);
    if (not std::filesystem::exists(full_path)) {
        throw std::runtime_error{full_path.string() + ": no such directory"};
    }
    if (not std::filesystem::is_directory(full_path)) {
        throw std::runtime_error{full_path.string() + ": is not a directory, cannot iterate over it"};
    }
    return iterate_directory_async(std::move(full_path));
}

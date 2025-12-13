#include "OverlayFilesystem.h"

#include <liboscar/Platform/NativeFilesystem.h>
#include <liboscar/Platform/ResourceDirectoryEntry.h>
#include <liboscar/Platform/ResourcePath.h>
#include <liboscar/Platform/ResourceStream.h>
#include <liboscar/Shims/Cpp23/generator.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <unordered_set>

using namespace osc;
namespace rgs = std::ranges;

std::optional<std::filesystem::path> osc::OverlayFilesystem::resource_filepath(const ResourcePath& resource_path) const
{
    for (auto& layer : layers_) {
        // `resource_filepath` is mostly a backwards-compatibility hack for
        // codebases that *must* load data from the native filesystem - other
        // implementations shouldn't support the feature, so we use a downcast
        // here.
        if (auto* native_filesystem = dynamic_cast<NativeFilesystem*>(layer.get())) {
            if (auto resource_filepath = native_filesystem->resource_filepath(resource_path)) {
                return resource_filepath;
            }
        }
    }
    return std::nullopt;
}

bool osc::OverlayFilesystem::impl_resource_exists(const ResourcePath& resource_path)
{
    return rgs::any_of(layers_, [&resource_path](auto& ptr) { return ptr->resource_exists(resource_path); });
}

osc::ResourceStream osc::OverlayFilesystem::impl_open(const ResourcePath& resource_path)
{
    auto it = rgs::find_if(layers_, [&resource_path](auto& ptr) { return ptr->resource_exists(resource_path); });
    if (it == rgs::end(layers_)) {
        throw std::runtime_error{resource_path.string() + ": no such resource found"};
    }
    return (*it)->open(resource_path);
}

osc::cpp23::generator<osc::ResourceDirectoryEntry> osc::OverlayFilesystem::impl_iterate_directory(ResourcePath resource_path)
{
    std::unordered_set<ResourceDirectoryEntry> previously_emitted_entries;
    for (auto& layer : layers_) {
        for (auto&& entry : layer->iterate_directory(resource_path)) {
            auto [it, inserted] = previously_emitted_entries.emplace(std::forward<decltype(entry)>(entry));
            if (inserted) {
                co_yield ResourceDirectoryEntry{*it};
            }
        }
    }
}

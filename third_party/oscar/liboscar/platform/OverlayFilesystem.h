#pragma once

#include <liboscar/platform/ResourceDirectoryEntry.h>
#include <liboscar/platform/ResourceStream.h>
#include <liboscar/platform/VirtualFilesystem.h>
#include <liboscar/shims/cpp23/generator.h>

#include <concepts>
#include <filesystem>
#include <functional>
#include <utility>
#include <vector>

namespace osc { class ResourcePath; }

namespace osc
{
    // A `VirtualFilesystem` that overlays a sequence of sub-`VirtualFilesystem`s, where
    // each one is handled in-turn when resolving a resource.
    class OverlayFilesystem final : public VirtualFilesystem {
    public:
        template<std::derived_from<VirtualFilesystem> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        VirtualFilesystem& emplace_lowest_priority(Args&&... args)
        {
            layers_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
            return *layers_.back();
        }

        std::optional<std::filesystem::path> resource_filepath(const ResourcePath&) const;
    private:
        bool impl_resource_exists(const ResourcePath&) final;
        ResourceStream impl_open(const ResourcePath&) final;
        cpp23::generator<ResourceDirectoryEntry> impl_iterate_directory(ResourcePath) final;

        std::vector<std::unique_ptr<VirtualFilesystem>> layers_;
    };
}

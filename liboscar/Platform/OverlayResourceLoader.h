#pragma once

#include <liboscar/Platform/IResourceLoader.h>
#include <liboscar/Platform/ResourceDirectoryEntry.h>
#include <liboscar/Platform/ResourceStream.h>
#include <liboscar/Shims/Cpp23/generator.h>

#include <concepts>
#include <filesystem>
#include <functional>
#include <utility>
#include <vector>

namespace osc { class ResourcePath; }

namespace osc
{
    // An `IResourceLoader` that overlays a sequence of sub-`IResourceLoader`s, where
    // each one is handled in-turn when resolving a resource.
    class OverlayResourceLoader final : public IResourceLoader {
    public:
        template<std::derived_from<IResourceLoader> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        IResourceLoader& emplace_lowest_priority(Args&&... args)
        {
            loaders_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
            return *loaders_.back();
        }

        std::optional<std::filesystem::path> resource_filepath(const ResourcePath&) const;
    private:
        bool impl_resource_exists(const ResourcePath&) final;
        ResourceStream impl_open(const ResourcePath&) final;
        cpp23::generator<ResourceDirectoryEntry> impl_iterate_directory(ResourcePath) final;

        std::vector<std::unique_ptr<IResourceLoader>> loaders_;
    };
}

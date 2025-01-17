#pragma once

#include <liboscar/Platform/IResourceLoader.h>
#include <liboscar/Platform/ResourceDirectoryEntry.h>
#include <liboscar/Platform/ResourceStream.h>

#include <filesystem>
#include <functional>

namespace osc { class ResourcePath; }

namespace osc
{
    class FilesystemResourceLoader final : public IResourceLoader {
    public:
        explicit FilesystemResourceLoader(const std::filesystem::path& root_directory) :
            root_directory_{root_directory}
        {}

    private:
        bool impl_resource_exists(const ResourcePath&) final;
        ResourceStream impl_open(const ResourcePath&) final;
        std::function<std::optional<ResourceDirectoryEntry>()> impl_iterate_directory(const ResourcePath&) final;

        std::filesystem::path root_directory_;
    };
}

#pragma once

#include <oscar/Platform/IResourceLoader.h>
#include <oscar/Platform/ResourceDirectoryEntry.h>
#include <oscar/Platform/ResourceStream.h>

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
        ResourceStream impl_open(const ResourcePath&) final;
        std::function<std::optional<ResourceDirectoryEntry>()> impl_iterate_directory(const ResourcePath&) final;

        std::filesystem::path root_directory_;
    };
}

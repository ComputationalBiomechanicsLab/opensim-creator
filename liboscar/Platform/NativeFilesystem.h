#pragma once

#include <liboscar/Platform/ResourceDirectoryEntry.h>
#include <liboscar/Platform/ResourceStream.h>
#include <liboscar/Platform/VirtualFilesystem.h>
#include <liboscar/Shims/Cpp23/generator.h>

#include <filesystem>
#include <functional>
#include <optional>

namespace osc { class ResourcePath; }

namespace osc
{
    // A `VirtualFilesystem` that uses the process's native filesystem.
    class NativeFilesystem final : public VirtualFilesystem {
    public:
        explicit NativeFilesystem(const std::filesystem::path& root_directory) :
            root_directory_{root_directory}
        {}

        const std::filesystem::path& root_directory() const { return root_directory_; }

        std::optional<std::filesystem::path> resource_filepath(const ResourcePath&) const;
    private:
        bool impl_resource_exists(const ResourcePath&) final;
        ResourceStream impl_open(const ResourcePath&) final;
        cpp23::generator<ResourceDirectoryEntry> impl_iterate_directory(ResourcePath) final;

        std::filesystem::path root_directory_;
    };
}

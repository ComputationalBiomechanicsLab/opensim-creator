#pragma once

#include <oscar/Platform/ResourceDirectoryEntry.h>
#include <oscar/Platform/ResourceStream.h>

#include <functional>
#include <optional>
#include <string>

namespace osc { class ResourcePath; }

namespace osc
{
    class IResourceLoader {
    protected:
        IResourceLoader() = default;
        IResourceLoader(const IResourceLoader&) = default;
        IResourceLoader(IResourceLoader&&) noexcept = default;
        IResourceLoader& operator=(const IResourceLoader&) = default;
        IResourceLoader& operator=(IResourceLoader&&) noexcept = default;
    public:
        virtual ~IResourceLoader() noexcept = default;

        ResourceStream open(const ResourcePath& resource_path) { return impl_open(resource_path); }
        std::string slurp(const ResourcePath&);

        std::function<std::optional<ResourceDirectoryEntry>()> iterate_directory(const ResourcePath& resource_path)
        {
            return impl_iterate_directory(resource_path);
        }

    private:
        virtual ResourceStream impl_open(const ResourcePath&) = 0;
        virtual std::function<std::optional<ResourceDirectoryEntry>()> impl_iterate_directory(const ResourcePath&)
        {
            // i.e. "can't iterate anything"
            return []() { return std::nullopt; };
        }
    };
}

#pragma once

#include <liboscar/platform/resource_directory_entry.h>
#include <liboscar/platform/resource_path.h>
#include <liboscar/platform/resource_stream.h>
#include <liboscar/platform/virtual_filesystem.h>
#include <liboscar/shims/cpp23/generator.h>

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace osc
{
    // An accessor that loads resources within a prefix (i.e. a directory)
    // relative to the root of some `VirtualFilesystem`.
    //
    // Example: An implementation might combine `VirtualFilesystem`s with
    // a `LayeredFilesystem` and prefix it with `textures/`, as part of a
    // "texture loader" subsystem.
    class ResourceLoader {
    public:
        explicit ResourceLoader(
            std::shared_ptr<VirtualFilesystem> impl,
            ResourcePath prefix = ResourcePath{}) :

            impl_{std::move(impl)},
            prefix_{std::move(prefix)}
        {}

        bool resource_exists(const ResourcePath& resource_path)
        {
            return impl_->resource_exists(resource_path);
        }

        ResourceStream open(const ResourcePath& resource_path)
        {
            return impl_->open(prefix_ / resource_path);
        }

        std::string slurp(const ResourcePath& resource_path)
        {
            return impl_->slurp(prefix_ / resource_path);
        }

        ResourceLoader with_prefix(const ResourcePath& prefix) const
        {
            return ResourceLoader{impl_, prefix_ / prefix};
        }

        template<typename StringLike>
        ResourceLoader with_prefix(StringLike&& str) const
            requires std::constructible_from<ResourceLoader, StringLike&&>
        {
            return with_prefix(ResourcePath{std::forward<StringLike>(str)});
        }

        cpp23::generator<ResourceDirectoryEntry> iterate_directory(const ResourcePath& resource_path)
        {
            return impl_->iterate_directory(prefix_ / resource_path);
        }
    private:
        template<std::derived_from<VirtualFilesystem> TResourceLoader, typename... Args>
        requires std::constructible_from<TResourceLoader, Args&&...>
        friend ResourceLoader make_resource_loader(Args&&...);

        std::shared_ptr<VirtualFilesystem> impl_;
        ResourcePath prefix_;
    };

    template<std::derived_from<VirtualFilesystem> TResourceLoader, typename... Args>
    requires std::constructible_from<TResourceLoader, Args&&...>
    ResourceLoader make_resource_loader(Args&&... args)
    {
        return ResourceLoader{std::make_shared<TResourceLoader>(std::forward<Args>(args)...)};
    }
}

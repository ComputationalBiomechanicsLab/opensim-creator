#pragma once

#include <liboscar/Platform/IResourceLoader.h>
#include <liboscar/Platform/ResourceDirectoryEntry.h>
#include <liboscar/Platform/ResourcePath.h>
#include <liboscar/Platform/ResourceStream.h>
#include <liboscar/Shims/Cpp23/generator.h>

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace osc
{
    class ResourceLoader {
    public:
        explicit ResourceLoader(
            std::shared_ptr<IResourceLoader> impl,
            ResourcePath prefix = ResourcePath{}) :

            impl_{std::move(impl)},
            prefix_{std::move(prefix)}
        {}

        operator IResourceLoader& () { return *impl_; }
        operator const IResourceLoader& () const { return *impl_; }

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
        template<std::derived_from<IResourceLoader> TResourceLoader, typename... Args>
        requires std::constructible_from<TResourceLoader, Args&&...>
        friend ResourceLoader make_resource_loader(Args&&...);

        std::shared_ptr<IResourceLoader> impl_;
        ResourcePath prefix_;
    };

    template<std::derived_from<IResourceLoader> TResourceLoader, typename... Args>
    requires std::constructible_from<TResourceLoader, Args&&...>
    ResourceLoader make_resource_loader(Args&&... args)
    {
        return ResourceLoader{std::make_shared<TResourceLoader>(std::forward<Args>(args)...)};
    }
}

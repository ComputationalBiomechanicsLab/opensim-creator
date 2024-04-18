#pragma once

#include <oscar/Platform/IResourceLoader.h>
#include <oscar/Platform/ResourceDirectoryEntry.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Platform/ResourceStream.h>

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
        operator IResourceLoader& () { return *m_Impl; }
        operator IResourceLoader const& () { return *m_Impl; }
        ResourceStream open(ResourcePath const& p) { return m_Impl->open(m_Prefix / p); }
        std::string slurp(ResourcePath const& p) { return m_Impl->slurp(m_Prefix / p); }
        ResourceLoader withPrefix(ResourcePath const& prefix) const { return ResourceLoader{m_Impl, m_Prefix / prefix}; }
        ResourceLoader withPrefix(std::string_view sv) { return withPrefix(ResourcePath{sv}); }
        std::function<std::optional<ResourceDirectoryEntry>()> iterate_directory(ResourcePath const& p) { return m_Impl->iterate_directory(m_Prefix / p); }

    private:
        template<
            std::derived_from<IResourceLoader> TResourceLoader,
            typename... Args
        >
        requires std::constructible_from<TResourceLoader, Args&&...>
        friend ResourceLoader make_resource_loader(Args&&...);

        explicit ResourceLoader(
            std::shared_ptr<IResourceLoader> impl_,
            ResourcePath const& prefix_ = ResourcePath{}) :

            m_Impl{std::move(impl_)},
            m_Prefix{prefix_}
        {}

        std::shared_ptr<IResourceLoader> m_Impl;
        ResourcePath m_Prefix;
    };

    template<
        std::derived_from<IResourceLoader> TResourceLoader,
        typename... Args
    >
    requires std::constructible_from<TResourceLoader, Args&&...>
    ResourceLoader make_resource_loader(Args&&... args)
    {
        return ResourceLoader{std::make_shared<TResourceLoader>(std::forward<Args>(args)...)};
    }
}

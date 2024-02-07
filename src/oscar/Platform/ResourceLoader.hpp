#pragma once

#include <oscar/Platform/IResourceLoader.hpp>
#include <oscar/Platform/ResourcePath.hpp>
#include <oscar/Platform/ResourceStream.hpp>

#include <concepts>
#include <memory>
#include <utility>

namespace osc
{
    class ResourceLoader {
    public:
        ResourceStream open(ResourcePath const& p) { return m_Impl->open(p); }
        std::string slurp(ResourcePath const&);
    private:
        template<
            std::derived_from<IResourceLoader> TResourceLoader,
            typename... Args
        >
        friend ResourceLoader make_resource_loader(Args&&...)
            requires std::constructible_from<TResourceLoader, Args&&...>;

        explicit ResourceLoader(std::shared_ptr<IResourceLoader> impl_) :
            m_Impl{std::move(impl_)}
        {}

        std::shared_ptr<IResourceLoader> m_Impl;
    };

    template<
        std::derived_from<IResourceLoader> TResourceLoader,
        typename... Args
    >
    ResourceLoader make_resource_loader(Args&&... args)
        requires std::constructible_from<TResourceLoader, Args&&...>
    {
        return ResourceLoader{std::make_shared<TResourceLoader>(std::forward<Args>(args)...)};
    }
}

#pragma once

#include <oscar/Platform/ResourcePath.hpp>
#include <oscar/Platform/ResourceStream.hpp>

namespace osc
{
    class IResourceLoader {
    protected:
        IResourceLoader() = default;
        IResourceLoader(IResourceLoader const&) = default;
        IResourceLoader(IResourceLoader&&) noexcept = default;
        IResourceLoader& operator=(IResourceLoader const&) = default;
        IResourceLoader& operator=(IResourceLoader&&) noexcept = default;
    public:
        virtual ~IResourceLoader() noexcept = default;

        ResourceStream open(ResourcePath const& p) { return implOpen(p); }

    private:
        virtual ResourceStream implOpen(ResourcePath const&) = 0;
    };
}

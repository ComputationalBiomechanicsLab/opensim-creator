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
        IResourceLoader(IResourceLoader const&) = default;
        IResourceLoader(IResourceLoader&&) noexcept = default;
        IResourceLoader& operator=(IResourceLoader const&) = default;
        IResourceLoader& operator=(IResourceLoader&&) noexcept = default;
    public:
        virtual ~IResourceLoader() noexcept = default;

        ResourceStream open(ResourcePath const& p) { return implOpen(p); }
        std::string slurp(ResourcePath const&);

        std::function<std::optional<ResourceDirectoryEntry>()> iterateDirectory(ResourcePath const& p)
        {
            return implIterateDirectory(p);
        }

    private:
        virtual ResourceStream implOpen(ResourcePath const&) = 0;
        virtual std::function<std::optional<ResourceDirectoryEntry>()> implIterateDirectory(ResourcePath const&)
        {
            // i.e. "can't iterate anything"
            return []() { return std::nullopt; };
        }
    };
}

#pragma once

#include <oscar/Platform/IResourceLoader.hpp>
#include <oscar/Platform/ResourceDirectoryEntry.hpp>
#include <oscar/Platform/ResourceStream.hpp>

#include <filesystem>
#include <functional>

namespace osc { class ResourcePath; }

namespace osc
{
    class FilesystemResourceLoader final : public IResourceLoader {
    public:
        FilesystemResourceLoader(std::filesystem::path const& root_) :
            m_Root{root_}
        {}

    private:
        ResourceStream implOpen(ResourcePath const&);
        std::function<std::optional<ResourceDirectoryEntry>()> implIterateDirectory(ResourcePath const&);

        std::filesystem::path m_Root;
    };
}

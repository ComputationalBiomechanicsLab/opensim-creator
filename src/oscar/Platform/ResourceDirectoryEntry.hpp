#pragma once

#include <oscar/Platform/ResourcePath.hpp>

#include <utility>

namespace osc
{
    class ResourceDirectoryEntry final {
    public:
        ResourceDirectoryEntry(ResourcePath path_, bool isDirectory_) :
            m_Path{std::move(path_)},
            m_IsDirectory{isDirectory_}
        {}

        ResourcePath const& path() const { return m_Path; }
        operator ResourcePath const& () const { return m_Path; }
        bool is_directory() const { return m_IsDirectory; }
    private:
        ResourcePath m_Path;
        bool m_IsDirectory;
    };
}

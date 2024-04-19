#pragma once

#include <oscar/Platform/ResourcePath.h>

#include <utility>

namespace osc
{
    class ResourceDirectoryEntry final {
    public:
        ResourceDirectoryEntry(ResourcePath path, bool is_directory) :
            path_{std::move(path)},
            is_directory_{is_directory}
        {}

        const ResourcePath& path() const { return path_; }
        operator const ResourcePath& () const { return path_; }
        bool is_directory() const { return is_directory_; }
    private:
        ResourcePath path_;
        bool is_directory_;
    };
}

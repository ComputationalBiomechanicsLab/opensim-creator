#pragma once

#include <liboscar/platform/ResourcePath.h>

#include <functional>
#include <iosfwd>
#include <utility>

namespace osc
{
    class ResourceDirectoryEntry final {
    public:
        ResourceDirectoryEntry(ResourcePath path, bool is_directory) :
            path_{std::move(path)},
            is_directory_{is_directory}
        {}

        friend bool operator==(const ResourceDirectoryEntry&, const ResourceDirectoryEntry&) = default;

        const ResourcePath& path() const { return path_; }
        operator const ResourcePath& () const { return path_; }
        bool is_directory() const { return is_directory_; }
    private:
        ResourcePath path_;
        bool is_directory_;
    };

    std::ostream& operator<<(std::ostream&, const ResourceDirectoryEntry&);
}

template<>
struct std::hash<osc::ResourceDirectoryEntry> final {
    size_t operator()(const osc::ResourceDirectoryEntry&) const;
};

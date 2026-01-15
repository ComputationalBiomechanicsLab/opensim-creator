#pragma once

#include <liboscar/platform/resource_directory_entry.h>
#include <liboscar/platform/resource_stream.h>
#include <liboscar/shims/cpp23/generator.h>

#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace osc { class ResourcePath; }

namespace osc
{
    // An abstract base class for an object that can access `ResourceStream`s
    // from an implementation-defined data source (e.g. filesystem, database,
    // zip file). Commonly called a VFS (Virtual File System) in operating
    // systems and game engines.
    class VirtualFilesystem {
    protected:
        VirtualFilesystem() = default;
        VirtualFilesystem(const VirtualFilesystem&) = default;
        VirtualFilesystem(VirtualFilesystem&&) noexcept = default;
        VirtualFilesystem& operator=(const VirtualFilesystem&) = default;
        VirtualFilesystem& operator=(VirtualFilesystem&&) noexcept = default;
    public:
        virtual ~VirtualFilesystem() noexcept = default;

        // Returns true if `resource_path` can be resolved by this `VirtualFilesystem`.
        bool resource_exists(const ResourcePath& resource_path) { return impl_resource_exists(resource_path); }

        // Returns a freshly-opened input stream to the data referenced by `resource_path`.
        //
        // - Throws if `resource_path` cannot be resolved by this `VirtualFilesystem`.
        // - Throws if `resource_path` refers to a directory.
        ResourceStream open(const ResourcePath& resource_path) { return impl_open(resource_path); }

        // Returns the entire contents of the input stream referenced by `resource_path` slurped
        // into a `std::string`.
        //
        // - Throws if `resource_path` cannot be resolved by this `VirtualFilesystem`.
        // - Throws if `resource_path` refers to a directory.
        std::string slurp(const ResourcePath& resource_path);

        // Returns a generator that yields entries of a directory referenced by
        // `resource_path` (does not recursively visit subdirectories).
        //
        // - The iteration order is implementation-defined.
        // - Each entry is visited only once.
        // - Throws if `resource_path` cannot be resolved by this `VirtualFilesystem`.
        // - Throws if `resource_path` is not a directory.
        cpp23::generator<ResourceDirectoryEntry> iterate_directory(ResourcePath resource_path)
        {
            return impl_iterate_directory(std::move(resource_path));
        }

    private:
        // Implementors must return `true` if `resource_path` can be resolved by this `VirtualFilesystem`
        // (i.e. a subsequent call to `impl_open`/`impl_iterate_directory` would succeed). Otherwise,
        // `false` must be returned.
        virtual bool impl_resource_exists(const ResourcePath& resource_path) = 0;

        // Implementors must return an opened `ResourceStream` that points to the first byte of the
        // resource referenced by `resource_path`. Otherwise, the impelementation must throw an exception.
        virtual ResourceStream impl_open(const ResourcePath& resource_path) = 0;

        // Implementors must return a generator that yields entries of a directory referenced by
        // `resource_path`, or throw an exception (see `iterate_directory` docstring for expected
        // behavior).
        virtual cpp23::generator<ResourceDirectoryEntry> impl_iterate_directory(ResourcePath resource_path) = 0;
    };
}

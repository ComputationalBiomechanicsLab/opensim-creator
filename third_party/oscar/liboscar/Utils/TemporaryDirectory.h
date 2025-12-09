#pragma once

#include <liboscar/Utils/TemporaryDirectoryParameters.h>

#include <filesystem>
#include <fstream>

namespace osc
{
    // `TemporaryDirectory` securely creates and manages a temporary directory.
    //
    // The implementation guarantees that:
    //
    // - The directory is created in the operating system's temporary directory, throwing
    //   otherwise.
    // - The name of the directory begins with `prefix`, ends with `suffix`, and the
    //   characters between those two are chosen to result in a new, unique, filename,
    //   throwing otherwise.
    // - The directory and all of its contents are removed from the filesystem upon
    //   destruction of the `TemporaryDirectory` object.
    class TemporaryDirectory final {
    public:
        // Constructs a `TemporaryDirectory` on-disk with the given parameters.
        explicit TemporaryDirectory(const TemporaryDirectoryParameters& = {});
        TemporaryDirectory(const TemporaryDirectory&) = delete;
        TemporaryDirectory(TemporaryDirectory&&) noexcept;
        TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
        TemporaryDirectory& operator=(TemporaryDirectory&&) noexcept;
        ~TemporaryDirectory() noexcept;

        // Returns the name of the temporary directory.
        std::filesystem::path filename() const { return absolute_path_.filename(); }

        // Returns the absolute path to the temporary directory.
        const std::filesystem::path& absolute_path() const { return absolute_path_; }

    private:
        std::filesystem::path absolute_path_;
        bool should_delete_ = true;
    };
}

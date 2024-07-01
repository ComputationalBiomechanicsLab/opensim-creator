#pragma once

#include <oscar/Utils/TemporaryFileParameters.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace osc
{
    // `TemporaryFile` creates and manages temporary files safely.
    //
    // The implementation guarantees that:
    //
    // - The file is created in the operating system's temporary directory, or `dir`, throwing otherwise
    // - The name of the temporary file begins with `prefix`, ends with `suffix`, and the characters between
    //   those two are are chosen to result in a new, unique, filename, throwing otherwise
    // - The file will be deleted from the filesystem upon destruction of the `TemporaryFile` object
    class TemporaryFile final {
    public:
        // constructs a `TemporaryFile` with the given parameters
        //
        // the file is created on-disk and opened by the constructor
        explicit TemporaryFile(const TemporaryFileParameters& = {});
        TemporaryFile(const TemporaryFile&) = delete;
        TemporaryFile(TemporaryFile&&) noexcept;
        TemporaryFile& operator=(const TemporaryFile&) = delete;
        TemporaryFile& operator=(TemporaryFile&&) noexcept;
        ~TemporaryFile() noexcept;

        // returns the name of the temporary file
        std::filesystem::path file_name() const { return absolute_path_.filename(); }

        // returns the absolute path to the temporary file
        const std::filesystem::path& absolute_path() const { return absolute_path_; }

        // returns the underlying stream that is connected to the temporary file
        std::fstream& stream() { return handle_; }

        // closes the handle that this `TemporaryFile` has to the underlying file, but does not delete
        // the underlying file (the destructor still deletes it, though)
        void close() { handle_.close(); }

    private:
        std::filesystem::path absolute_path_;
        std::fstream handle_;
        bool should_delete_ = true;
    };
}

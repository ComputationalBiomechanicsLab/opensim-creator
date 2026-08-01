#include "resource_stream.h"

#include <liboscar/utilities/exception_helpers.h>

#include <filesystem>
#include <fstream>
#include <memory>

using namespace osc;

namespace
{
    std::unique_ptr<std::istream> open_stream_or_throw(const std::filesystem::path& path)
    {
        // This pre-check is necessary because MacOS actually allows opening
        // a `std::ifstream` to a directory because it handles all paths as
        // valid file descriptors.
        if (std::filesystem::is_directory(path)) {
            throw formatted_runtime_error("{}: is a directory, not a file", path.string());
        }

        auto rv = std::make_unique<std::ifstream>(path, std::ios::binary | std::ios::in);
        if (not *rv) {
            throw formatted_runtime_error("{}: failed to load ResourceStream", path.string());
        }
        return rv;
    }
}

osc::ResourceStream::ResourceStream() :
    name_{"nullstream"},
    handle_{std::make_unique<std::ifstream>()}
{}

osc::ResourceStream::ResourceStream(const std::filesystem::path& path_) :
    name_{path_.filename().string()},
    handle_{open_stream_or_throw(path_)}
{}

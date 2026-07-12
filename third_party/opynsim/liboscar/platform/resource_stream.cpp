#include "resource_stream.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace
{
    std::unique_ptr<std::istream> open_stream_or_throw(const std::filesystem::path& path)
    {
        // This pre-check is necessary because MacOS actually allows opening
        // a `std::ifstream` to a directory because it handles all paths as
        // valid file descriptors.
        if (std::filesystem::is_directory(path)) {
            throw std::runtime_error{std::format("{}: is a directory, not a file", path.string())};
        }

        auto rv = std::make_unique<std::ifstream>(path, std::ios::binary | std::ios::in);
        if (not *rv) {
            throw std::runtime_error{std::format("{}: failed to load ResourceStream", path.string())};
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

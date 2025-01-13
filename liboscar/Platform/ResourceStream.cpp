#include "ResourceStream.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <utility>

namespace
{
    std::unique_ptr<std::istream> open_stream_or_throw(const std::filesystem::path& path)
    {
        auto rv = std::make_unique<std::ifstream>(path, std::ios::binary | std::ios::in);
        if (not (*rv)) {
            std::stringstream ss;
            ss << path << ": failed to load ResourceStream";
            throw std::runtime_error{std::move(ss).str()};
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

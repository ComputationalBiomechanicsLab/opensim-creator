#include "ResourceStream.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <utility>

namespace
{
    std::unique_ptr<std::istream> OpenFileStreamOrThrow(std::filesystem::path const& path)
    {
        auto rv = std::make_unique<std::ifstream>(path, std::ios::binary | std::ios::in);
        if (!(*rv))
        {
            std::stringstream ss;
            ss << path << ": failed to load icon file";
            throw std::runtime_error{std::move(ss).str()};
        }
        return rv;
    }
}

osc::ResourceStream::ResourceStream(std::filesystem::path const& path_) :
    m_Handle{OpenFileStreamOrThrow(path_)}
{}

#pragma once

#include <filesystem>

namespace osmv::cfg {
    std::filesystem::path resource_path(std::filesystem::path const&);
    std::filesystem::path shader_path(char const* shader_name);
}

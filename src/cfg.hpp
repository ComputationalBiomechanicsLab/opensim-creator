#pragma once

#include <filesystem>

namespace osmv::cfg {
    std::filesystem::path resource_path(std::filesystem::path const&);

    template<typename ...Els>
    std::filesystem::path resource_path(Els... els) {
        std::filesystem::path p;
        (p /= ... /= els);
        return resource_path(p);
    }

    std::filesystem::path shader_path(char const* shader_name);
}

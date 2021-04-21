#pragma once

#include <filesystem>
#include <string>

namespace osc {
    std::string slurp_into_string(std::filesystem::path const&);
}

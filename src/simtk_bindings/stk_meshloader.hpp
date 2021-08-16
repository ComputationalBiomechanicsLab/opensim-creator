#pragma once

#include "src/3d/model.hpp"

#include <filesystem>

namespace osc {
    NewMesh stk_load_mesh(std::filesystem::path const&);
}

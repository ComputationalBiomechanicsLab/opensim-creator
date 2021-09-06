#pragma once

#include "src/3d/Model.hpp"

#include <filesystem>

namespace osc {
    Mesh SimTKLoadMesh(std::filesystem::path const&);
}

#pragma once

#include "src/3D/Model.hpp"

#include <filesystem>

namespace osc {
    Mesh SimTKLoadMesh(std::filesystem::path const&);
}

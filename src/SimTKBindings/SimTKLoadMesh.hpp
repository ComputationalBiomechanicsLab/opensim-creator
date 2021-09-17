#pragma once

#include "src/3D/Model.hpp"

#include <filesystem>

namespace osc {
    MeshData SimTKLoadMesh(std::filesystem::path const&);
}

#pragma once

#include "src/3D/Model.hpp"

#include <filesystem>

namespace osc {
    CPUMesh SimTKLoadMesh(std::filesystem::path const&);
}

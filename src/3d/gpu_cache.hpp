#pragma once

#include "gpu_storage.hpp"

#include <unordered_map>

namespace osmv {
    struct Gpu_cache final {
        Gpu_storage storage;
        std::unordered_map<std::string, Mesh_reference> filepath2mesh;

        Mesh_reference simbody_sphere;
        Mesh_reference simbody_cylinder;
        Mesh_reference simbody_cube;
        Mesh_reference floor_quad;

        Texture_reference chequered_texture;

        Gpu_cache();
    };
}

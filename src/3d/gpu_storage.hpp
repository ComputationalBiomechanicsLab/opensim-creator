#pragma once

#include "mesh_storage.hpp"
#include "texture_storage.hpp"

namespace osmv {
    struct Gpu_storage final {
        Mesh_storage meshes;
        Texture_storage textures;
    };
}

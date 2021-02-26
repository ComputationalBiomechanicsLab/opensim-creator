#pragma once

#include "src/3d/mesh_storage.hpp"
#include "src/3d/shader_cache.hpp"
#include "src/3d/texture_storage.hpp"

namespace osmv {
    struct Gpu_storage final {
        Shader_cache shaders;
        Mesh_storage meshes;
        Texture_storage textures;
    };
}

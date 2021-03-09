#pragma once

#include "src/3d/gpu_data_reference.hpp"
#include "src/3d/gpu_storage.hpp"

#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace osmv {
    struct Gpu_cache final {
        Gpu_storage storage;
        std::unordered_map<std::string, Mesh_reference> filepath2mesh;

        Mesh_reference simbody_sphere;
        Mesh_reference simbody_cylinder;
        Mesh_reference simbody_cube;
        Mesh_reference floor_quad;
        Mesh_reference _25x25grid;
        Mesh_reference y_line;  // 2 verts @ x = 0, y = [-1.0f, 1.0f], z = 0

        Texture_reference chequered_texture;

        Gpu_cache();

        template<typename MeshCreator>
        [[nodiscard]] Mesh_reference lookup_or_construct_mesh(std::string const& k, MeshCreator f) {
            auto [it, inserted] = filepath2mesh.emplace(
                std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(Mesh_reference::invalid()));

            if (not inserted) {
                return it->second;
            }

            Mesh_reference ref = storage.meshes.allocate(f());
            it->second = ref;
            return ref;
        }
    };
}

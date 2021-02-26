#include "gpu_cache.hpp"

#include "src/3d/mesh_generation.hpp"
#include "src/3d/mesh_storage.hpp"
#include "src/3d/texture_storage.hpp"
#include "src/3d/textured_vert.hpp"
#include "src/3d/texturing.hpp"
#include "src/3d/untextured_vert.hpp"

#include <glm/vec2.hpp>

#include <array>
#include <vector>

osmv::Gpu_cache::Gpu_cache() :
    simbody_sphere{storage.meshes.allocate(unit_sphere_triangles())},
    simbody_cylinder{storage.meshes.allocate(simbody_cylinder_triangles())},
    simbody_cube{storage.meshes.allocate(simbody_brick_triangles())},
    floor_quad{[& meshes = this->storage.meshes]() {
        auto quad_verts = osmv::shaded_textured_quad_verts();
        for (Textured_vert& v : quad_verts) {
            v.texcoord *= 200.0f;
        }
        return meshes.allocate(quad_verts);
    }()},

    chequered_texture{storage.textures.allocate(generate_chequered_floor_texture())} {
}

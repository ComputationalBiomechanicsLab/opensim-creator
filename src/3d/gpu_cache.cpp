#include "gpu_cache.hpp"

#include "src/3d/mesh_generation.hpp"
#include "src/3d/texturing.hpp"

osmv::Gpu_cache::Gpu_cache() :
    storage{},
    filepath2mesh{},

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

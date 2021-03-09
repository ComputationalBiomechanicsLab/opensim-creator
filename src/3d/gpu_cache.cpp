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

using namespace osmv;

[[nodiscard]] static Textured_mesh generate_floor_quad() {
    auto quad_verts = osmv::shaded_textured_quad_verts();
    for (Textured_vert& v : quad_verts.vert_data) {
        v.texcoord *= 200.0f;
    }
    return quad_verts;
}

template<size_t N>
[[nodiscard]] static Plain_mesh generate_NxN_grid() {
    static constexpr float z = 0.0f;
    static constexpr float min = -1.0f;
    static constexpr float max = 1.0f;
    static constexpr size_t lines_per_dimension = N;
    static constexpr float step_size = (max - min) / static_cast<float>(lines_per_dimension - 1);
    static constexpr size_t num_lines = 2 * lines_per_dimension;
    static constexpr size_t num_points = 2 * num_lines;

    std::array<Untextured_vert, num_points> verts;
    glm::vec3 normal = {0.0f, 0.0f, 0.0f};  // same for all

    size_t idx = 0;

    // lines parallel to X axis
    for (size_t i = 0; i < lines_per_dimension; ++i) {
        float y = min + i * step_size;

        Untextured_vert& p1 = verts[idx++];
        p1.pos = {-1.0f, y, z};
        p1.normal = normal;

        Untextured_vert& p2 = verts[idx++];
        p2.pos = {1.0f, y, z};
        p2.normal = normal;
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < lines_per_dimension; ++i) {
        float x = min + i * step_size;

        Untextured_vert& p1 = verts[idx++];
        p1.pos = {x, -1.0f, z};
        p1.normal = normal;

        Untextured_vert& p2 = verts[idx++];
        p2.pos = {x, 1.0f, z};
        p2.normal = normal;
    }

    return Plain_mesh::from_raw_verts(verts);
}

[[nodiscard]] static Plain_mesh generate_y_line() {
    std::array<Untextured_vert, 2> points = {{// pos                // normal
                                              {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
                                              {{0.0f, +1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}}};
    return Plain_mesh::from_raw_verts(points);
}

osmv::Gpu_cache::Gpu_cache() :
    storage{},
    filepath2mesh{},

    simbody_sphere{storage.meshes.allocate(unit_sphere_triangles())},
    simbody_cylinder{storage.meshes.allocate(simbody_cylinder_triangles())},
    simbody_cube{storage.meshes.allocate(simbody_brick_triangles())},
    floor_quad{storage.meshes.allocate(generate_floor_quad())},
    _25x25grid{storage.meshes.allocate(generate_NxN_grid<25>())},
    y_line{storage.meshes.allocate(generate_y_line())},

    chequered_texture{storage.textures.allocate(generate_chequered_floor_texture())} {
}

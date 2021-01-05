#pragma once

#include "gl_extensions.hpp"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <vector>
#include <array>
#include <cmath>

// meshes: basic methods for generating triangle meshes of primitives

namespace osmv {
    static constexpr float pi_f = static_cast<float>(M_PI);

    struct Shaded_textured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };
    static_assert(sizeof(Shaded_textured_vert) == 8*sizeof(float));

    // standard textured quad
    // - dimensions [-1, +1] in xy and [0, 0] in z
    // - uv coords are (0, 0) bottom-left, (1, 1) top-right
    // - normal is +1 in Z, meaning that it faces toward the camera
    static constexpr std::array<Shaded_textured_vert, 6> shaded_textured_quad_verts = {{
        {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
        {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
        {{ 1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
        {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
        {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
        {{-1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}  // top-left
    }};

    // one vertex of a mesh
    //
    // a triangle mesh contains some multiple of 3 of these vertices
    struct Untextured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
    };
    static_assert(sizeof(glm::vec3) == 3*sizeof(float), "unexpected struct size: could cause problems when uploading to the GPU");
    static_assert(sizeof(Untextured_vert) == 6*sizeof(float), "unexpected struct size: could cause problems when uploading to the GPU");

    // Returns triangles of a "unit" (radius = 1.0f, origin = 0,0,0) sphere
    void unit_sphere_triangles(std::vector<Untextured_vert>& out);

    // Returns triangles for a "unit" cylinder with `num_sides` sides.
    //
    // Here, "unit" means:
    //
    // - radius == 1.0f
    // - top == [0.0f, 0.0f, -1.0f]
    // - bottom == [0.0f, 0.0f, +1.0f]
    // - (so the height is 2.0f, not 1.0f)
    void unit_cylinder_triangles(size_t num_sides, std::vector<osmv::Untextured_vert>& out);

    // Returns triangles for a "simbody" cylinder with `num_sides` sides.
    //
    // This matches simbody-visualizer.cpp's definition of a cylinder, which
    // is:
    //
    // radius
    //     1.0f
    // top
    //     [0.0f, 1.0f, 0.0f]
    // bottom
    //     [0.0f, -1.0f, 0.0f]
    //
    // see simbody-visualizer.cpp::makeCylinder for my source material
    void simbody_cylinder_triangles(size_t num_sides, std::vector<osmv::Untextured_vert>& out);

    gl::Texture_2d generate_chequered_floor_texture();
}

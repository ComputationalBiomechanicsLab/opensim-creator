#pragma once

#include <vector>

// meshes: basic methods for generating triangle meshes of primitives

namespace osmv {
    // Vector of 3 floats with no padding, so that it can be passed to OpenGL
    struct Vec3 {
        float x;
        float y;
        float z;
        constexpr Vec3(float _x, float _y, float _z) : x{_x}, y{_y}, z{_z} {
        }
    };

    struct Mesh_point {
        Vec3 position;
        Vec3 normal;
    };

    // Returns triangles of a "unit" (radius = 1.0f, origin = 0,0,0) sphere
    std::vector<Mesh_point> unit_sphere_triangles();

    // Returns triangles for a "unit" cylinder with `num_sides` sides.
    //
    // Here, "unit" means:
    //
    // - radius == 1.0f
    // - top == [0.0f, 0.0f, -1.0f]
    // - bottom == [0.0f, 0.0f, +1.0f]
    // - (so the height is 2.0f, not 1.0f)
    std::vector<Mesh_point> unit_cylinder_triangles(size_t num_sides);

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
    std::vector<Mesh_point> simbody_cylinder_triangles(size_t num_sides);
}

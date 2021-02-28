#pragma once

#include "src/3d/mesh.hpp"

// 3d common: common primitives/structs used for mesh generation/rendering
namespace osmv {
    [[nodiscard]] Textured_mesh shaded_textured_quad_verts();

    // Returns triangles of a "unit" (radius = 1.0f, origin = 0,0,0) sphere
    [[nodiscard]] Plain_mesh unit_sphere_triangles();

    // Returns triangles for a "unit" cylinder with `num_sides` sides.
    //
    // Here, "unit" means:
    //
    // - radius == 1.0f
    // - top == [0.0f, 0.0f, -1.0f]
    // - bottom == [0.0f, 0.0f, +1.0f]
    // - (so the height is 2.0f, not 1.0f)
    [[nodiscard]] Plain_mesh unit_cylinder_triangles();

    // Returns triangles for a standard "simbody" cylinder
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
    [[nodiscard]] Plain_mesh simbody_cylinder_triangles();

    // Returns triangles for a standard "Simbody" cube
    //
    // TODO: I have no idea what a Simbody cube is, the verts returned by this are
    // a pure guess
    [[nodiscard]] Plain_mesh simbody_brick_triangles();
}

#pragma once

#include "opensim_wrapper.hpp"

#include <vector>

// meshes: basic methods for generating triangle meshes of primitives

namespace osmv {
    // Returns triangles of a "unit" (radius = 1.0f, origin = 0,0,0) sphere
    void unit_sphere_triangles(std::vector<osim::Untextured_triangle>& out);

    // Returns triangles for a "unit" cylinder with `num_sides` sides.
    //
    // Here, "unit" means:
    //
    // - radius == 1.0f
    // - top == [0.0f, 0.0f, -1.0f]
    // - bottom == [0.0f, 0.0f, +1.0f]
    // - (so the height is 2.0f, not 1.0f)
    void unit_cylinder_triangles(size_t num_sides, std::vector<osim::Untextured_triangle>& out);

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
    void simbody_cylinder_triangles(size_t num_sides, std::vector<osim::Untextured_triangle>& out);
}

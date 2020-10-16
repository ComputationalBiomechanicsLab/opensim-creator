#include "meshes.hpp"

#include <cmath>
#include <stdexcept>

constexpr float pi_f = M_PI;

using osmv::Vec3;

static Vec3 normals(Vec3 const& p1, Vec3 const& p2, Vec3 const& p3) {
    //https://stackoverflow.com/questions/19350792/calculate-normal-of-a-single-triangle-in-3d-space/23709352
    Vec3 a = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
    Vec3 b = { p3.x - p1.x, p3.y - p1.y, p3.z - p1.z };

    float x = a.y * b.z - a.z * b.y;
    float y = a.z * b.x - a.x * b.z;
    float z = a.x * b.y - a.y * b.x;

    return {x,y,z};
}


// Returns triangles of a "unit" (radius = 1.0f, origin = 0,0,0) sphere
std::vector<osmv::Mesh_point> osmv::unit_sphere_triangles() {
    // this is a shitty alg that produces a shitty UV sphere. I don't have
    // enough time to implement something better, like an isosphere, or
    // something like a patched sphere:
    //
    // https://www.iquilezles.org/www/articles/patchedsphere/patchedsphere.htm
    //
    // This one is adapted from:
    //    http://www.songho.ca/opengl/gl_sphere.html#example_cubesphere

    size_t sectors = 12;
    size_t stacks = 12;

    // polar coords, with [0, 0, -1] pointing towards the screen with polar
    // coords theta = 0, phi = 0. The coordinate [0, 1, 0] is theta = (any)
    // phi = PI/2. The coordinate [1, 0, 0] is theta = PI/2, phi = 0
    std::vector<Mesh_point> points;

    float theta_step = 2.0f*pi_f / sectors;
    float phi_step = pi_f / stacks;

    for (size_t stack = 0; stack <= stacks; ++stack) {
        float phi = pi_f/2.0f - static_cast<float>(stack)*phi_step;
        float y = sin(phi);

        for (unsigned sector = 0; sector <= sectors; ++sector) {
            float theta = sector * theta_step;
            float x = sin(theta) * cos(phi);
            float z = -cos(theta) * cos(phi);
            points.push_back(Mesh_point{
                .position = {x, y, z},
                .normal = {x, y, z},  // sphere is at the origin, so nothing fancy needed
            });
        }
    }

    // the points are not triangles. They are *points of a triangle*, so the
    // points must be triangulated
    std::vector<Mesh_point> triangles;

    for (size_t stack = 0; stack < stacks; ++stack) {
        size_t k1 = stack * (sectors + 1);
        size_t k2 = k1 + sectors + 1;

        for (size_t sector = 0; sector < sectors; ++sector, ++k1, ++k2) {
            // 2 triangles per sector - excluding the first and last stacks
            // (which contain one triangle, at the poles)
            Mesh_point p1 = points.at(k1);
            Mesh_point p2 = points.at(k2);
            Mesh_point p1_plus1 = points.at(k1+1u);
            Mesh_point p2_plus1 = points.at(k2+1u);

            if (stack != 0) {
                triangles.push_back(p1);
                triangles.push_back(p1_plus1);
                triangles.push_back(p2);
            }

            if (stack != (stacks-1)) {
                triangles.push_back(p1_plus1);
                triangles.push_back(p2_plus1);
                triangles.push_back(p2);                
            }
        }
    }

    return triangles;
}

// Returns triangles for a "unit" cylinder with `num_sides` sides.
//
// Here, "unit" means:
//
// - radius == 1.0f
// - top == [0.0f, 0.0f, -1.0f]
// - bottom == [0.0f, 0.0f, +1.0f]
// - (so the height is 2.0f, not 1.0f)
std::vector<osmv::Mesh_point> osmv::unit_cylinder_triangles(size_t num_sides) {
    // TODO: this is dumb because a cylinder can be EBO-ed quite easily, which
    //       would reduce the amount of vertices needed
    if (num_sides < 3) {
        throw std::runtime_error{"cannot create a cylinder with fewer than 3 sides"};
    }

    std::vector<Mesh_point> rv;
    rv.reserve(2u*3u*num_sides + 6u*num_sides);

    float step_angle = (2.0f*pi_f)/num_sides;
    float top_z = -1.0f;
    float bottom_z = +1.0f;

    // top
    {
        Vec3 p1 = {0.0f, 0.0f, top_z};  // middle
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i*step_angle;
            float theta_end = (i+1)*step_angle;
            Vec3 p2(sin(theta_start), cos(theta_start), top_z);
            Vec3 p3(sin(theta_end), cos(theta_end), top_z);
            Vec3 normal = normals(p1, p2, p3);
            rv.push_back({p1, normal});
            rv.push_back({p2, normal});
            rv.push_back({p3, normal});
        }
    }

    // bottom
    {
        Vec3 p1 = {0.0f, 0.0f, -1.0f};  // middle
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i*step_angle;
            float theta_end = (i+1)*step_angle;

            Vec3 p2(sin(theta_start), cos(theta_start), bottom_z);
            Vec3 p3(sin(theta_end), cos(theta_end), bottom_z);
            Vec3 normal = normals(p1, p2, p3);

            rv.push_back({p1, normal});
            rv.push_back({p3, normal});
            rv.push_back({p3, normal});
        }
    }

    // sides
    {
        float norm_start = step_angle/2.0f;
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = theta_start + step_angle;
            float norm_theta = theta_start + norm_start;

            Vec3 p1(sin(theta_start), cos(theta_start), top_z);
            Vec3 p2(sin(theta_end), cos(theta_end), top_z);
            Vec3 p3(sin(theta_start), cos(theta_start), bottom_z);
            Vec3 p4(sin(theta_start), cos(theta_start), bottom_z);

            // triangle 1
            Vec3 n1 = normals(p1, p2, p3);
            rv.push_back({p1, n1});
            rv.push_back({p2, n1});
            rv.push_back({p3, n1});

            // triangle 2
            Vec3 n2 = normals(p3, p4, p2);
            rv.push_back({p3, n2});
            rv.push_back({p4, n2});
            rv.push_back({p2, n2});
        }
    }

    return rv;
}

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
std::vector<osmv::Mesh_point> osmv::simbody_cylinder_triangles(size_t num_sides) {
    // TODO: this is dumb because a cylinder can be EBO-ed quite easily, which
    //       would reduce the amount of vertices needed
    if (num_sides < 3) {
        throw std::runtime_error{"cannot create a cylinder with fewer than 3 sides"};
    }

    std::vector<Mesh_point> rv;
    rv.reserve(2*3*num_sides + 6*num_sides);

    float step_angle = (2.0f*pi_f)/num_sides;
    float top_y = +1.0f;
    float bottom_y = -1.0f;

    // top
    {
        Vec3 normal = {0.0f, 1.0f, 0.0f};
        Mesh_point top_middle = {
            .position = {0.0f, top_y, 0.0f},
            .normal = normal,
        };
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i*step_angle;
            float theta_end = (i+1)*step_angle;

            // note: these should be wound counter-clockwise, for backface
            // culling.
            rv.push_back(top_middle);
            rv.push_back(Mesh_point {
                .position = Vec3(cos(theta_end), top_y, sin(theta_end)),
                .normal = normal,
            });
            rv.push_back(Mesh_point {
                .position = Vec3(cos(theta_start), top_y, sin(theta_start)),
                .normal = normal,
            });
        }
    }

    // bottom
    {
        Vec3 bottom_normal = {0.0f, -1.0f, 0.0f};
        Mesh_point top_middle = {
            .position = {0.0f, bottom_y, 0.0f},
            .normal = bottom_normal,
        };
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i*step_angle;
            float theta_end = (i+1)*step_angle;

            // note: these should be wound counter-clockwise, for backface
            // culling.
            rv.push_back(top_middle);
            rv.push_back(Mesh_point {
                .position = Vec3(cos(theta_start), bottom_y, sin(theta_start)),
                .normal = bottom_normal,
            });
            rv.push_back(Mesh_point {
                .position = Vec3(cos(theta_end), bottom_y, sin(theta_end)),
                .normal = bottom_normal,
            });
        }
    }

    // sides
    {
        float norm_start = step_angle/2.0f;
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = theta_start + step_angle;
            float norm_theta = theta_start + norm_start;

            Vec3 normal = Vec3(cos(norm_theta), 0.0f, sin(norm_theta));
            Vec3 top1 = Vec3(cos(theta_start), top_y, sin(theta_start));
            Vec3 top2 = Vec3(cos(theta_end), top_y, sin(theta_end));

            Vec3 bottom1 = top1;
            bottom1.y = bottom_y;
            Vec3 bottom2 = top2;
            bottom2.y = bottom_y;

            // draw 2 triangles per rectangular side

            // note: these should be wound counter-clockwise, for backface
            // culling.
            rv.push_back(Mesh_point{top1, normal});
            rv.push_back(Mesh_point{top2, normal});
            rv.push_back(Mesh_point{bottom1, normal});

            rv.push_back(Mesh_point{bottom2, normal});
            rv.push_back(Mesh_point{bottom1, normal});
            rv.push_back(Mesh_point{top2, normal});
        }
    }

    return rv;
}

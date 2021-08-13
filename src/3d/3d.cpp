#include "src/3d/3d.hpp"

#include "src/utils/stbi_wrapper.hpp"
#include "src/utils/shims.hpp"
#include "src/3d/gl_glm.hpp"

#include <glm/gtx/norm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <iostream>
#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <cmath>

using namespace osc;

static inline constexpr float pi_f = osc::numbers::pi_v<float>;

// standard textured cube with dimensions [-1, +1] in xyz and uv coords of
// (0, 0) bottom-left, (1, 1) top-right for each (quad) face
static constexpr std::array<Textured_vert, 36> g_ShadedTexturedCubeVerts = {{

    // back face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},  // top-left

    // front face
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left

    // left face
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-right
    {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
    {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-right
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-right

    // right face
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-left
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-right
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-left
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-left

    // bottom face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},  // top-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-left
    {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // top-right

    // top face
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top-left
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}  // bottom-left
}};

// standard textured quad
// - dimensions [-1, +1] in xy and [0, 0] in z
// - uv coords are (0, 0) bottom-left, (1, 1) top-right
// - normal is +1 in Z, meaning that it faces toward the camera
static constexpr std::array<Textured_vert, 6> g_ShadedTexturedQuadVerts = {{

    // CCW winding (culling)
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right

    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
}};

// a cube wire mesh, suitable for GL_LINES drawing
//
// a pair of verts per edge of the cube. The cube has 12 edges, so 24 lines
static constexpr std::array<Untextured_vert, 24> g_CubeEdgeLines = {{
    // back

    // back bottom left -> back bottom right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{+1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back bottom right -> back top right
    {{+1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{+1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back top right -> back top left
    {{+1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{-1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back top left -> back bottom left
    {{-1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // front

    // front bottom left -> front bottom right
    {{-1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{+1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front bottom right -> front top right
    {{+1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front top right -> front top left
    {{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{-1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front top left -> front bottom left
    {{-1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{-1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front-to-back edges

    // front bottom left -> back bottom left
    {{-1.0f, -1.0f, +1.0f}, {-1.0f, -1.0f, +1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}},

    // front bottom right -> back bottom right
    {{+1.0f, -1.0f, +1.0f}, {+1.0f, -1.0f, +1.0f}},
    {{+1.0f, -1.0f, -1.0f}, {+1.0f, -1.0f, -1.0f}},

    // front top left -> back top left
    {{-1.0f, +1.0f, +1.0f}, {-1.0f, +1.0f, +1.0f}},
    {{-1.0f, +1.0f, -1.0f}, {-1.0f, +1.0f, -1.0f}},

    // front top right -> back top right
    {{+1.0f, +1.0f, +1.0f}, {+1.0f, +1.0f, +1.0f}},
    {{+1.0f, +1.0f, -1.0f}, {+1.0f, +1.0f, -1.0f}}
}};

// private implementation details
namespace {

    // generate 1:1 indices for each vert in a CPU_mesh
    //
    // effectively, generate the indices [0, 1, ..., size-1]
    template<typename TVert>
    void generate_1to1_indices_for_verts(Mesh<TVert>& mesh) {

        // edge-case: ensure the caller isn't trying to used oversized indices
        if (mesh.verts.size() > std::numeric_limits<GLushort>::max()) {
            throw std::runtime_error{"cannot generate indices for a mesh: the mesh has too many vertices: if you need to support this many vertices then contact the developers"};
        }

        size_t n = mesh.verts.size();
        mesh.indices.resize(n);
        for (size_t i = 0; i < n; ++i) {
            mesh.indices[i] = static_cast<GLushort>(i);
        }
    }

    // compute an AABB from a sequence of vertices in 3D space
    template<typename TVert, typename Getter>
    [[nodiscard]] constexpr AABB aabb_compute_from_verts(TVert const* vs, size_t n, Getter getter) noexcept {
        AABB rv{};

        // edge-case: no points provided
        if (n == 0) {
            rv.min = {0.0f, 0.0f, 0.0f};
            rv.max = {0.0f, 0.0f, 0.0f};
            return rv;
        }

        rv.min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        rv.max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

        // otherwise, compute bounds
        for (size_t i = 0; i < n; ++i) {
            glm::vec3 const& pos = getter(vs[i]);

            rv.min[0] = std::min(rv.min[0], pos[0]);
            rv.min[1] = std::min(rv.min[1], pos[1]);
            rv.min[2] = std::min(rv.min[2], pos[2]);

            rv.max[0] = std::max(rv.max[0], pos[0]);
            rv.max[1] = std::max(rv.max[1], pos[1]);
            rv.max[2] = std::max(rv.max[2], pos[2]);
        }

        return rv;
    }

    // hackily computes the bounding sphere around verts
    //
    // see: https://en.wikipedia.org/wiki/Bounding_sphere for better algs
    template<typename TVert, typename Getter>
    [[nodiscard]] constexpr Sphere sphere_compute_bounding_sphere_from_verts(TVert const* vs, size_t n, Getter getter) noexcept {
        AABB aabb = aabb_compute_from_verts(vs, n, getter);

        Sphere rv{};
        rv.origin = (aabb.min + aabb.max) / 2.0f;
        rv.radius = 0.0f;

        // edge-case: no points provided
        if (n == 0) {
            return rv;
        }

        float biggest_r2 = 0.0f;
        for (size_t i = 0; i < n; ++i) {
            glm::vec3 const& pos = getter(vs[i]);
            float r2 = glm::distance2(rv.origin, pos);
            biggest_r2 = std::max(biggest_r2, r2);
        }

        rv.radius = glm::sqrt(biggest_r2);

        return rv;
    }

    // helper method: load a file into an image and send it to OpenGL
    void load_cubemap_surface(char const* path, GLenum target) {
        auto img = osc::stbi::Image::load(path);

        if (!img) {
            std::stringstream ss;
            ss << path << ": error loading cubemap surface: " << osc::stbi::failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        GLenum format;
        if (img->channels == 1) {
            format = GL_RED;
        } else if (img->channels == 3) {
            format = GL_RGB;
        } else if (img->channels == 4) {
            format = GL_RGBA;
        } else {
            std::stringstream msg;
            msg << path << ": error: contains " << img->channels
                << " color channels (the implementation doesn't know how to handle this)";
            throw std::runtime_error{std::move(msg).str()};
        }

        glTexImage2D(target, 0, format, img->width, img->height, 0, format, GL_UNSIGNED_BYTE, img->data);
    }
}

// public API

std::ostream& osc::operator<<(std::ostream& o, glm::vec2 const& v) {
    return o << '(' << v.x << ", " << v.y << ')';
}

std::ostream& osc::operator<<(std::ostream& o, glm::vec3 const& v) {
    return o << '(' << v.x << ", " << v.y << ", " << v.z << ')';
}

std::ostream& osc::operator<<(std::ostream& o, glm::vec4 const& v) {
    return o << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')';
}

bool osc::is_colocated(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    float eps = std::numeric_limits<float>::epsilon();
    float eps2 = eps * eps;
    float len2 = glm::length2(a - b);
    return len2 > eps2;
}

glm::vec3 osc::vec_min(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    return glm::vec3{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

glm::vec3 osc::vec_max(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    return glm::vec3{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

glm::vec3::length_type osc::vec_longest_dim_idx(glm::vec3 const& v) noexcept {
    if (v.x > v.y && v.x > v.z) {
        return 0;  // X is longest
    } else if (v.y > v.z) {
        return 1;  // Y is longest
    } else {
        return 2;  // Z is longest
    }
}

glm::vec3 osc::triangle_normal(glm::vec3 const& a, glm::vec3 const& b, glm::vec3 const& c) noexcept {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 perpendiular = glm::cross(ab, ac);
    return glm::normalize(perpendiular);
}

glm::mat3 osc::normal_matrix(glm::mat4 const& m) noexcept {
    glm::mat3 top_left{m};
    return glm::inverse(glm::transpose(top_left));
}

glm::mat3 osc::normal_matrix(glm::mat4x3 const& m) noexcept {
    glm::mat3 top_left{m};
    return glm::inverse(glm::transpose(top_left));
}


// AABB impl.

std::ostream& osc::operator<<(std::ostream& o, AABB const& aabb) {
    return o << "p1 = " << aabb.min << ", p2 = " << aabb.max;
}

glm::vec3 osc::aabb_center(AABB const& a) noexcept {
    return (a.min + a.max)/2.0f;
}

glm::vec3 osc::aabb_dims(AABB const& a) noexcept {
    return a.max - a.min;
}

AABB osc::aabb_union(AABB const& a, AABB const& b) noexcept {
    return AABB{vec_min(a.min, b.min), vec_max(a.max, b.max)};
}

bool osc::aabb_is_empty(AABB const& a) noexcept {
    glm::vec3 dims = aabb_dims(a);
    float volume = dims.x * dims.y * dims.z;
    return volume <= std::numeric_limits<float>::epsilon();
}

glm::vec3::length_type osc::aabb_longest_dim_idx(AABB const& a) noexcept {
    return vec_longest_dim_idx(aabb_dims(a));
}

std::array<glm::vec3, 8> osc::aabb_verts(AABB const& aabb) noexcept {

    glm::vec3 d = aabb_dims(aabb);

    // effectively, span each dimension from each AABB corner
    return std::array<glm::vec3, 8>{{
        aabb.min,
        aabb.min + d.x,
        aabb.min + d.y,
        aabb.min + d.z,
        aabb.max,
        aabb.max - d.x,
        aabb.max - d.y,
        aabb.max - d.z,
    }};
}


AABB osc::aabb_xform(AABB const& aabb, glm::mat4 const& m) noexcept {
    auto verts = aabb_verts(aabb);
    for (auto& vert : verts) {
        vert = m * glm::vec4{vert, 1.0f};
    }

    AABB rv{verts[0], verts[0]};
    for (size_t i = 1; i < verts.size(); ++i) {
        rv.min = vec_min(rv.min, verts[i]);
        rv.max = vec_max(rv.max, verts[i]);
    }
    return rv;
}

osc::AABB osc::aabb_from_mesh(Untextured_mesh const& m) noexcept {
    return aabb_compute_from_verts(m.verts.data(), m.verts.size(), [](auto const& v) { return v.pos; });
}

AABB osc::aabb_from_points(glm::vec3 const* first, size_t n) {
    return aabb_compute_from_verts(first, n, [](auto const& v) { return v; });
}

AABB osc::aabb_from_triangle(glm::vec3 const* first) {
    return aabb_compute_from_verts(first, 3, [](auto const& v) { return v; });
}


// analytical geometry

std::ostream& osc::operator<<(std::ostream& o, Sphere const& s) {
    return o << "origin = " << s.origin << ", radius = " << s.radius;
}

std::ostream& osc::operator<<(std::ostream& o, Line const& l) {
    return o << "origin = " << l.o << ", direction = " << l.d;
}

std::ostream& osc::operator<<(std::ostream& o, Plane const& p) {
    return o << "origin = " << p.origin << ", normal = " << p.normal;
}

std::ostream& osc::operator<<(std::ostream& o, Disc const& d) {
    return o << "origin = " << d.origin << ", normal = " << d.normal << ", radius = " << d.radius;
}

osc::Sphere osc::sphere_bounds_of_mesh(Untextured_mesh const& m) noexcept {
    return sphere_compute_bounding_sphere_from_verts(m.verts.data(), m.verts.size(), [](auto const& v) { return v.pos; });
}

osc::Sphere osc::sphere_bounds_of_points(glm::vec3 const* ps, size_t n) noexcept {
    return sphere_compute_bounding_sphere_from_verts(ps, n, [](auto const& p) { return p; });
}

osc::AABB osc::sphere_aabb(Sphere const& s) noexcept {
    AABB rv;
    rv.min = s.origin - s.radius;
    rv.max = s.origin + s.radius;
    return rv;
}

glm::mat4 osc::circle_to_disc_xform(Disc const& d) noexcept {
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {d.radius, d.radius, 1.0f});

    glm::vec3 discNormal = {0.0f, 0.0f, 1.0f};     // just how the generator defines it
    float angle = glm::acos(glm::dot(discNormal, d.normal));  // dot(a, b) == |a||b|cos(ang)
    glm::vec3 axis = glm::cross(discNormal, d.normal);

    glm::mat4 rotator = glm::rotate(glm::mat4{1.0f}, angle, axis);
    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, d.origin);

    return translator * rotator * scaler;
}


// hit testing impl.

struct Quadratic_formula_result final {
    bool computeable;
    float x0;
    float x1;
};

// solve a quadratic formula
//
// only real-valued results supported - no complex-plane results
static Quadratic_formula_result solveQuadratic(float a, float b, float c) {
    Quadratic_formula_result res;

    // b2 - 4ac
    float discriminant = b*b - 4.0f*a*c;

    if (discriminant < 0.0f) {
        res.computeable = false;
        return res;
    }

    // q = -1/2 * (b +- sqrt(b2 - 4ac))
    float q = -0.5f * (b + copysign(sqrt(discriminant), b));

    // you might be wondering why this doesn't just compute a textbook
    // version of the quadratic equation (-b +- sqrt(disc))/2a
    //
    // the reason is because `-b +- sqrt(b2 - 4ac)` can result in catastrophic
    // cancellation if `-b` is close to `sqrt(disc)`
    //
    // so, instead, we use two similar, complementing, quadratics:
    //
    // the textbook one:
    //
    //     x = (-b +- sqrt(disc)) / 2a
    //
    // and the "Muller's method" one:
    //
    //     x = 2c / (-b -+ sqrt(disc))
    //
    // the great thing about these two is that the "+-" part of their
    // equations are complements, so you can have:
    //
    // q = -0.5 * (b + sign(b)*sqrt(disc))
    //
    // which, handily, will only *accumulate* the sum inside those
    // parentheses. If `b` is positive, you end up with a positive
    // number. If `b` is negative, you end up with a negative number. No
    // catastropic cancellation. By multiplying it by "-0.5" you end up
    // with:
    //
    //     -b - sqrt(disc)
    //
    // or, if B was negative:
    //
    //     -b + sqrt(disc)
    //
    // both of which are valid terms of both the quadratic equations above
    //
    // see:
    //
    //     https://math.stackexchange.com/questions/1340267/alternative-quadratic-formula
    //     https://en.wikipedia.org/wiki/Quadratic_equation


    res.computeable = true;
    res.x0 = q/a;  // textbook "complete the square" equation
    res.x1 = c/q;  // Muller's method equation
    return res;
}

static Line_sphere_hittest_result line_intersects_sphere_geometric(Sphere const& s, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    Line_sphere_hittest_result rv;

    glm::vec3 L = s.origin - l.o;  // line origin to sphere origin
    float tca = glm::dot(L, l.d);  // projected line from middle of hitline to sphere origin

    if (tca < 0.0f) {
        // line is pointing away from the sphere
        rv.intersected = false;
        return rv;
    }

    float d2 = glm::dot(L, L) - tca*tca;
    float r2 = s.radius * s.radius;

    if (d2 > r2) {
        // line is not within the sphere's radius
        rv.intersected = false;
        return rv;
    }

    // the collision points are on the sphere's surface (R), and D
    // is how far the hitline midpoint is from the radius. Can use
    // Pythag to figure out the midpoint length (thc)
    float thc = glm::sqrt(r2 - d2);

    rv.t0 = tca - thc;
    rv.t1 = tca + thc;
    rv.intersected = true;
    return rv;
}

static Line_sphere_hittest_result line_intersects_sphere_analytic(Sphere const& s, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    Line_sphere_hittest_result rv;

    glm::vec3 L = l.o - s.origin;

    // coefficients of the quadratic implicit:
    //
    //     P2 - R2 = 0
    //     (O + tD)2 - R2 = 0
    //     (O + tD - C)2 - R2 = 0
    //
    // where:
    //
    //     P    a point on the surface of the sphere
    //     R    the radius of the sphere
    //     O    origin of line
    //     t    scaling factor for line direction (we want this)
    //     D    direction of line
    //     C    center of sphere
    //
    // if the quadratic has solutions, then there must exist one or two
    // `t`s that are points on the sphere's surface.

    float a = glm::dot(l.d, l.d);  // always == 1.0f if d is normalized
    float b = 2.0f * glm::dot(l.d, L);
    float c = glm::dot(L, L) - glm::dot(s.radius, s.radius);

    auto [ok, t0, t1] = solveQuadratic(a, b, c);

    if (!ok) {
        rv.intersected = false;
        return rv;
    }

    // ensure X0 < X1
    if (t0 > t1) {
        std::swap(t0, t1);
    }

    // ensure it's in front
    if (t0 < 0.0f) {
        t0 = t1;
        if (t0 < 0.0f) {
            rv.intersected = false;
            return rv;
        }
    }

    rv.t0 = t0;
    rv.t1 = t1;
    rv.intersected = true;

    return rv;
}

Line_sphere_hittest_result osc::line_intersects_sphere(Sphere const& s, Line const& l) noexcept {
    return line_intersects_sphere_analytic(s, l);
}

Line_AABB_hittest_result osc::line_intersects_AABB(AABB const& aabb, Line const& l) noexcept {
    Line_AABB_hittest_result rv;

    float t0 = -FLT_MAX;
    float t1 = FLT_MAX;

    // intersect the ray with each axis-aligned slab for each dimension
    //
    // i.e. figure out where the line intersects the front+back of the AABB
    //      in (e.g.) X, then Y, then Z, and intersect those interactions such
    //      that if the intersection is ever empty (or, negative here) then there
    //      is no intersection
    for (int i = 0; i < 3; ++i) {
        float invDir = 1.0f / l.d[i];
        float tNear = (aabb.min[i] - l.o[i]) * invDir;
        float tFar = (aabb.max[i] - l.o[i]) * invDir;
        if (tNear > tFar) {
            std::swap(tNear, tFar);
        }
        t0 = std::max(t0, tNear);
        t1 = std::min(t1, tFar);

        if (t0 > t1) {
            rv.intersected = false;
            return rv;
        }
    }

    rv.t0 = t0;
    rv.t1 = t1;
    rv.intersected = true;
    return rv;
}

Line_plane_hittest_result osc::line_intersects_plane(Plane const& p, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
    //
    // effectively, this is evaluating:
    //
    //     P, a point on the plane
    //     P0, the plane's origin (distance from world origin)
    //     N, the plane's normal
    //
    // against: dot(P-P0, N)
    //
    // which must equal zero for any point in the plane. Given that, a line can
    // be parameterized as `P = O + tD` where:
    //
    //     P, point along the line
    //     O, origin of line
    //     t, distance along line direction
    //     D, line direction
    //
    // sub the line equation into the plane equation, rearrange for `t` and you
    // can figure out how far a plane is along a line
    //
    // equation: t = dot(P0 - O, n) / dot(D, n)

    Line_plane_hittest_result rv;

    float denominator = glm::dot(p.normal, l.d);

    if (std::abs(denominator) > 1e-6) {
        float numerator = glm::dot(p.origin - l.o, p.normal);
        rv.intersected = true;
        rv.t = numerator / denominator;
        return rv;
    } else {
        // the line is *very* parallel to the plane, which could cause
        // some divide-by-zero havok: pretend it didn't intersect
        rv.intersected = false;
        return rv;
    }
}

Line_disc_hittest_result osc::line_intersects_disc(Disc const& d, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection

    // think of this as a ray-plane intersection test with the additional
    // constraint that the ray has to be within the radius of the disc

    Line_disc_hittest_result rv;

    Plane p;
    p.origin = d.origin;
    p.normal = d.normal;

    auto [ok, t] = line_intersects_plane(p, l);

    if (!ok) {
        rv.intersected = false;
        return rv;
    }

    glm::vec3 pos = l.o + t*l.d;
    glm::vec3 v = pos - d.origin;

    float d2 = glm::dot(v, v);
    float r2 = glm::dot(d.radius, d.radius);

    if (d2 > r2) {
        rv.intersected = false;
        return rv;
    }

    rv.intersected = true;
    rv.t = t;
    return rv;
}

Line_triangle_hittest_result osc::line_intersects_triangle(glm::vec3 const* v, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution

    Line_triangle_hittest_result rv;

    // compute triangle normal
    glm::vec3 N = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));

    // compute dot product between normal and ray
    float NdotR = glm::dot(N, l.d);

    // if the dot product is small, then the ray is probably very parallel to
    // the triangle and doesn't intersect
    if (std::abs(NdotR) < std::numeric_limits<float>::epsilon()) {
        rv.intersected = false;
        return rv;
    }

    // D is the distance between the origin and (assumed) point on the plane (v[0])
    float D = glm::dot(N, v[0]);
    float t = -(glm::dot(N, l.o) - D) / NdotR;

    // if triangle plane is behind line then return early
    if (t < 0.0f) {
        rv.intersected = false;
        return rv;
    }

    // intersection point on triangle plane, computed from line equation
    glm::vec3 P = l.o + t*l.d;

    // figure out if that point is inside the triangle's bounds using the
    // "inside-outside" test

    // test each triangle edge: {0, 1}, {1, 2}, {2, 0}
    for (int i = 0; i < 3; ++i) {
        glm::vec3 const& start = v[i];
        glm::vec3 const& end = v[(i+1)%3];

        // corner[n] to corner[n+1]
        glm::vec3 e = end - start;

        // corner[n] to P
        glm::vec3 c = P - start;

        // cross product of the above indicates whether the vectors are
        // clockwise or anti-clockwise with respect to eachover. It's a
        // right-handed coord system, so anti-clockwise produces a vector
        // that points in same direction as normal
        glm::vec3 ax = glm::cross(e, c);

        // if the dot product of that axis with the normal is <0.0f then
        // the point was "outside"
        if (glm::dot(ax, N) < 0.0f) {
            rv.intersected = false;
            return rv;
        }
    }

    rv.intersected = true;
    rv.t = t;
    return rv;
}






// generators

template<>
void osc::generate_banner(Textured_mesh& out) {
    out.clear();

    for (Textured_vert const& v : g_ShadedTexturedQuadVerts) {
        out.verts.emplace_back(v);
    }

    generate_1to1_indices_for_verts(out);
}

template<>
void osc::generate_uv_sphere(Untextured_mesh& out) {
    out.clear();

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
    std::vector<Untextured_vert> points;

    float theta_step = 2.0f * pi_f / sectors;
    float phi_step = pi_f / stacks;

    for (size_t stack = 0; stack <= stacks; ++stack) {
        float phi = pi_f / 2.0f - static_cast<float>(stack) * phi_step;
        float y = sinf(phi);

        for (unsigned sector = 0; sector <= sectors; ++sector) {
            float theta = sector * theta_step;
            float x = sinf(theta) * cosf(phi);
            float z = -cosf(theta) * cosf(phi);
            glm::vec3 pos{x, y, z};
            glm::vec3 normal{pos};
            points.push_back({pos, normal});
        }
    }

    // the points are not triangles. They are *points of a triangle*, so the
    // points must be triangulated

    for (size_t stack = 0; stack < stacks; ++stack) {
        size_t k1 = stack * (sectors + 1);
        size_t k2 = k1 + sectors + 1;

        for (size_t sector = 0; sector < sectors; ++sector, ++k1, ++k2) {
            // 2 triangles per sector - excluding the first and last stacks
            // (which contain one triangle, at the poles)
            Untextured_vert p1 = points.at(k1);
            Untextured_vert p2 = points.at(k2);
            Untextured_vert p1_plus1 = points.at(k1 + 1u);
            Untextured_vert p2_plus1 = points.at(k2 + 1u);

            if (stack != 0) {
                out.verts.push_back(p1);
                out.verts.push_back(p1_plus1);
                out.verts.push_back(p2);
            }

            if (stack != (stacks - 1)) {
                out.verts.push_back(p1_plus1);
                out.verts.push_back(p2_plus1);
                out.verts.push_back(p2);
            }
        }
    }

    generate_1to1_indices_for_verts(out);
}

template<>
void osc::generate_uv_sphere(std::vector<glm::vec3>& out) {
    auto shaded_verts = generate_uv_sphere<Untextured_mesh>();

    out.clear();
    out.reserve(shaded_verts.verts.size());
    for (auto const& v : shaded_verts.verts) {
        out.push_back(v.pos);
    }
}

void osc::generate_simbody_cylinder(size_t num_sides, Untextured_mesh& out) {

    out.clear();
    out.verts.reserve(2 * num_sides + 2 * num_sides);

    float step_angle = (2.0f * pi_f) / num_sides;
    float top_y = +1.0f;
    float bottom_y = -1.0f;

    // top
    {
        glm::vec3 normal = {0.0f, 1.0f, 0.0f};
        Untextured_vert top_middle{{0.0f, top_y, 0.0f}, normal};
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            // note: these are wound CCW for backface culling
            out.verts.push_back(top_middle);
            out.verts.push_back({glm::vec3(cosf(theta_end), top_y, sinf(theta_end)), normal});
            out.verts.push_back({glm::vec3(cosf(theta_start), top_y, sinf(theta_start)), normal});
        }
    }

    // bottom
    {
        glm::vec3 bottom_normal{0.0f, -1.0f, 0.0f};
        Untextured_vert top_middle{{0.0f, bottom_y, 0.0f}, bottom_normal};
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            // note: these are wound CCW for backface culling
            out.verts.push_back(top_middle);
            out.verts.push_back({
                glm::vec3(cosf(theta_start), bottom_y, sinf(theta_start)),
                bottom_normal,
            });
            out.verts.push_back({
                glm::vec3(cosf(theta_end), bottom_y, sinf(theta_end)),
                bottom_normal,
            });
        }
    }

    // sides
    {
        float norm_start = step_angle / 2.0f;
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = theta_start + step_angle;
            float norm_theta = theta_start + norm_start;

            glm::vec3 normal(cosf(norm_theta), 0.0f, sinf(norm_theta));
            glm::vec3 top1(cosf(theta_start), top_y, sinf(theta_start));
            glm::vec3 top2(cosf(theta_end), top_y, sinf(theta_end));

            glm::vec3 bottom1 = top1;
            bottom1.y = bottom_y;
            glm::vec3 bottom2 = top2;
            bottom2.y = bottom_y;

            // draw 2 triangles per quad cylinder side
            //
            // note: these are wound CCW for backface culling
            out.verts.push_back({top1, normal});
            out.verts.push_back({top2, normal});
            out.verts.push_back({bottom1, normal});

            out.verts.push_back({bottom2, normal});
            out.verts.push_back({bottom1, normal});
            out.verts.push_back({top2, normal});
        }
    }

    generate_1to1_indices_for_verts(out);
}

void osc::generate_NxN_grid(size_t n, Untextured_mesh& out) {
    static constexpr float z = 0.0f;
    static constexpr float min = -1.0f;
    static constexpr float max = 1.0f;

    size_t lines_per_dimension = n;
    float step_size = (max - min) / static_cast<float>(lines_per_dimension - 1);
    size_t num_lines = 2 * lines_per_dimension;
    size_t num_points = 2 * num_lines;

    out.clear();
    out.verts.resize(num_points);

    glm::vec3 normal = {0.0f, 0.0f, 0.0f};  // same for all

    size_t idx = 0;

    // lines parallel to X axis
    for (size_t i = 0; i < lines_per_dimension; ++i) {
        float y = min + i * step_size;

        Untextured_vert& p1 = out.verts[idx++];
        p1.pos = {-1.0f, y, z};
        p1.normal = normal;

        Untextured_vert& p2 = out.verts[idx++];
        p2.pos = {1.0f, y, z};
        p2.normal = normal;
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < lines_per_dimension; ++i) {
        float x = min + i * step_size;

        Untextured_vert& p1 = out.verts[idx++];
        p1.pos = {x, -1.0f, z};
        p1.normal = normal;

        Untextured_vert& p2 = out.verts[idx++];
        p2.pos = {x, 1.0f, z};
        p2.normal = normal;
    }

    generate_1to1_indices_for_verts(out);
}

void osc::generate_y_line(Untextured_mesh& out) {
    out.clear();
    out.verts.push_back({{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
    out.verts.push_back({{0.0f, +1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
    generate_1to1_indices_for_verts(out);
}

Untextured_mesh osc::generate_y_line() {
    Untextured_mesh m;
    generate_y_line(m);
    return m;
}

void osc::generate_cube_lines(Untextured_mesh& out) {
    out.clear();
    out.indices.reserve(g_CubeEdgeLines.size());
    out.verts.reserve(g_CubeEdgeLines.size());

    for (size_t i = 0; i < g_CubeEdgeLines.size(); ++i) {
        out.indices.push_back(static_cast<GLushort>(i));
        out.verts.push_back(g_CubeEdgeLines[i]);
    }
}

Untextured_mesh osc::generate_cube_lines() {
    Untextured_mesh m;
    generate_cube_lines(m);
    return m;
}

std::vector<glm::vec3> osc::generate_cube_lines_points() {
    Untextured_mesh m = generate_cube_lines();
    std::vector<glm::vec3> points;
    points.reserve(m.verts.size());
    for (auto const& v : m.verts) {
        points.push_back(v.pos);
    }
    return points;
}

std::vector<glm::vec3> osc::generate_circle_tris(size_t segments) {
    std::vector<glm::vec3> out;

    float fsegments = static_cast<float>(segments);
    float step = (2 * pi_f) / fsegments;

    out.clear();
    out.reserve(3*segments);
    for (size_t i = 0; i < segments; ++i) {
        float theta1 = i * step;
        float theta2 = (i+1) * step;

        out.emplace_back(0.0f, 0.0f, 0.0f);
        out.emplace_back(sinf(theta1), cosf(theta1), 0.0f);
        out.emplace_back(sinf(theta2), cosf(theta2), 0.0f);
    }

    return out;
}

template<>
void osc::generate_cube(Untextured_mesh& out) {
    out.clear();

    for (Textured_vert v : g_ShadedTexturedCubeVerts) {
        out.verts.push_back({v.pos, v.normal});
    }

    generate_1to1_indices_for_verts(out);
}

gl::Texture_2d osc::generate_chequered_floor_texture() {
    struct Rgb {
        unsigned char r, g, b;
    };
    constexpr size_t chequer_width = 32;
    constexpr size_t chequer_height = 32;
    constexpr size_t w = 2 * chequer_width;
    constexpr size_t h = 2 * chequer_height;
    constexpr Rgb on_color = {0xe5, 0xe5, 0xe5};
    constexpr Rgb off_color = {0xde, 0xde, 0xde};

    std::array<Rgb, w * h> pixels;
    for (size_t row = 0; row < h; ++row) {
        size_t row_start = row * w;
        bool y_on = (row / chequer_height) % 2 == 0;
        for (size_t col = 0; col < w; ++col) {
            bool x_on = (col / chequer_width) % 2 == 0;
            pixels[row_start + col] = y_on ^ x_on ? on_color : off_color;
        }
    }

    gl::Texture_2d rv;
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(rv.type, rv.handle());
    glTexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(rv.type);
    gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return rv;
}

Image_texture osc::load_image_as_texture(char const* path, Tex_flags flags) {
    gl::Texture_2d t;

    if (flags & TexFlag_Flip_Pixels_Vertically) {
        stbi::set_flip_vertically_on_load(true);
    }

    auto img = stbi::Image::load(path);

    if (!img) {
        std::stringstream ss;
        ss << path << ": error loading image: " << stbi::failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }

    if (flags & TexFlag_Flip_Pixels_Vertically) {
        stbi::set_flip_vertically_on_load(false);
    }

    GLenum internalFormat;
    GLenum format;
    if (img->channels == 1) {
        internalFormat = GL_RED;
        format = GL_RED;
    } else if (img->channels == 3) {
        internalFormat = flags & TexFlag_SRGB ? GL_SRGB : GL_RGB;
        format = GL_RGB;
    } else if (img->channels == 4) {
        internalFormat = flags & TexFlag_SRGB ? GL_SRGB_ALPHA : GL_RGBA;
        format = GL_RGBA;
    } else {
        std::stringstream msg;
        msg << path << ": error: contains " << img->channels
            << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    gl::BindTexture(t.type, t.handle());
    gl::TexImage2D(t.type, 0, internalFormat, img->width, img->height, 0, format, GL_UNSIGNED_BYTE, img->data);
    glGenerateMipmap(t.type);

    return Image_texture{std::move(t), img->width, img->height, img->channels};
}

gl::Texture_cubemap osc::load_cubemap(
    char const* path_pos_x,
    char const* path_neg_x,
    char const* path_pos_y,
    char const* path_neg_y,
    char const* path_pos_z,
    char const* path_neg_z) {

    stbi::set_flip_vertically_on_load(false);

    gl::Texture_cubemap rv;
    gl::BindTexture(rv);

    load_cubemap_surface(path_pos_x, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    load_cubemap_surface(path_neg_x, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    load_cubemap_surface(path_pos_y, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    load_cubemap_surface(path_neg_y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    load_cubemap_surface(path_pos_z, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    load_cubemap_surface(path_neg_z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    /**
     * From: https://learnopengl.com/Advanced-OpenGL/Cubemaps
     *
     * Don't be scared by the GL_TEXTURE_WRAP_R, this simply sets the wrapping
     * method for the texture's R coordinate which corresponds to the texture's
     * 3rd dimension (like z for positions). We set the wrapping method to
     * GL_CLAMP_TO_EDGE since texture coordinates that are exactly between two
     * faces may not hit an exact face (due to some hardware limitations) so
     * by using GL_CLAMP_TO_EDGE OpenGL always returns their edge values
     * whenever we sample between faces.
     */
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return rv;
}

void osc::Polar_perspective_camera::do_pan(float aspect_ratio, glm::vec2 delta) noexcept {
    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    float x_amt = delta.x * aspect_ratio * (2.0f * tanf(fov / 2.0f) * radius);
    float y_amt = -delta.y * (1.0f / aspect_ratio) * (2.0f * tanf(fov / 2.0f) * radius);

    // this assumes the scene is not rotated, so we need to rotate these
    // axes to match the scene's rotation
    glm::vec4 default_panning_axis = {x_amt, y_amt, 0.0f, 1.0f};
    auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto theta_vec = glm::normalize(glm::vec3{sinf(theta), 0.0f, cosf(theta)});
    auto phi_axis = glm::cross(theta_vec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), phi, phi_axis);

    glm::vec4 panning_axes = rot_phi * rot_theta * default_panning_axis;
    pan.x += panning_axes.x;
    pan.y += panning_axes.y;
    pan.z += panning_axes.z;
}

void osc::Polar_perspective_camera::do_drag(glm::vec2 delta) noexcept {
    theta += 2.0f * pi_f * -delta.x;
    phi += 2.0f * pi_f * delta.y;
}

void osc::Polar_perspective_camera::do_znear_zfar_autoscale() noexcept {
    // znear and zfar are only really dicated by the camera's radius, because
    // the radius is effectively the distance from the camera's focal point

    znear = 0.02f * radius;
    zfar = 20.0f * radius;
}

glm::mat4 osc::Polar_perspective_camera::view_matrix() const noexcept {
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    // this maths is a complete shitshow and I apologize. It just happens to work for now. It's a polar coordinate
    // system that shifts the world based on the camera pan

    auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto theta_vec = glm::normalize(glm::vec3{sinf(theta), 0.0f, cosf(theta)});
    auto phi_axis = glm::cross(theta_vec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), -phi, phi_axis);
    auto pan_translate = glm::translate(glm::identity<glm::mat4>(), pan);
    return glm::lookAt(glm::vec3(0.0f, 0.0f, radius), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3{0.0f, 1.0f, 0.0f}) *
           rot_theta * rot_phi * pan_translate;
}

glm::mat4 osc::Polar_perspective_camera::projection_matrix(float aspect_ratio) const noexcept {
    return glm::perspective(fov, aspect_ratio, znear, zfar);
}

glm::vec3 osc::Polar_perspective_camera::pos() const noexcept {
    float x = radius * sinf(theta) * cosf(phi);
    float y = radius * sinf(phi);
    float z = radius * cosf(theta) * cosf(phi);

    return -pan + glm::vec3{x, y, z};
}

osc::Euler_perspective_camera::Euler_perspective_camera() :
    pos{0.0f, 0.0f, 0.0f},
    pitch{0.0f},
    yaw{-pi_f/2.0f},
    fov{pi_f * 70.0f/180.0f},
    znear{0.1f},
    zfar{1000.0f} {
}

glm::vec3 osc::Euler_perspective_camera::front() const noexcept {
    return glm::normalize(glm::vec3{
        cosf(yaw) * cosf(pitch),
        sinf(pitch),
        sinf(yaw) * cosf(pitch),
    });
}

glm::vec3 osc::Euler_perspective_camera::up() const noexcept {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 osc::Euler_perspective_camera::right() const noexcept {
    return glm::normalize(glm::cross(front(), up()));
}

glm::mat4 osc::Euler_perspective_camera::view_matrix() const noexcept {
    return glm::lookAt(pos, pos + front(), up());
}

glm::mat4 osc::Euler_perspective_camera::projection_matrix(float aspect_ratio) const noexcept {
    return glm::perspective(fov, aspect_ratio, znear, zfar);
}

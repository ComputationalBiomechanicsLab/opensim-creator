#include "src/3d/model.hpp"

#include "src/3d/constants.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <iostream>
#include <cstddef>
#include <limits>

using namespace osc;

namespace {
    struct Untextured_vert {
        glm::vec3 pos;
        glm::vec3 norm;
    };

    struct Textured_vert {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 uv;
    };
}

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


static Ray_collision ray_collision_sphere_geometric(Sphere const& s, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    Ray_collision rv;

    glm::vec3 L = s.origin - l.o;  // line origin to sphere origin
    float tca = glm::dot(L, l.d);  // projected line from middle of hitline to sphere origin

    if (tca < 0.0f) {
        // line is pointing away from the sphere
        rv.hit = false;
        return rv;
    }

    float d2 = glm::dot(L, L) - tca*tca;
    float r2 = s.radius * s.radius;

    if (d2 > r2) {
        // line is not within the sphere's radius
        rv.hit = false;
        return rv;
    }

    // the collision points are on the sphere's surface (R), and D
    // is how far the hitline midpoint is from the radius. Can use
    // Pythag to figure out the midpoint length (thc)
    float thc = glm::sqrt(r2 - d2);

    rv.hit = true;
    rv.distance = tca - thc;  // other hit: tca + thc

    return rv;
}

static Ray_collision ray_collision_sphere_analytic(Sphere const& s, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    Ray_collision rv;

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
        rv.hit = false;
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
            rv.hit = false;
            return rv;
        }
    }

    rv.hit = true;
    rv.distance = t0;  // other = t1
    return rv;
}


// public API

std::ostream& osc::operator<<(std::ostream& o, glm::vec2 const& v) {
    return o << "vec2(" << v.x << ", " << v.y << ')';
}

std::ostream& osc::operator<<(std::ostream& o, glm::vec3 const& v) {
    return o << "vec3(" << v.x << ", " << v.y << ", " << v.z << ')';
}

std::ostream& osc::operator<<(std::ostream& o, glm::vec4 const& v) {
    return o << "vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')';
}

std::ostream& osc::operator<<(std::ostream& o, glm::mat3 const& m) {
    // prints in row-major, because that's how most people debug matrices
    for (int row = 0; row < 3; ++row) {
        char const* delim = "";
        for (int col = 0; col < 3; ++col) {
            o << delim << m[col][row];
            delim = " ";
        }
        o << '\n';
    }
    return o;
}

std::ostream& osc::operator<<(std::ostream& o, glm::mat4x3 const& m) {
    // prints in row-major, because that's how most people debug matrices
    for (int row = 0; row < 3; ++row) {
        char const* delim = "";
        for (int col = 0; col < 4; ++col) {
            o << delim << m[col][row];
            delim = " ";
        }
        o << '\n';
    }
    return o;
}

std::ostream& osc::operator<<(std::ostream& o, glm::mat4 const& m) {
    // prints in row-major, because that's how most people debug matrices
    for (int row = 0; row < 4; ++row) {
        char const* delim = "";
        for (int col = 0; col < 4; ++col) {
            o << delim << m[col][row];
            delim = " ";
        }
        o << '\n';
    }
    return o;
}

bool osc::is_colocated(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    float eps = std::numeric_limits<float>::epsilon();
    float eps2 = eps * eps;
    glm::vec3 b2a = a - b;
    float len2 = glm::dot(b2a, b2a);
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

float osc::vec_longest_dim(glm::vec3 const& v) noexcept {
    return v[vec_longest_dim_idx(v)];
}

glm::vec3 osc::triangle_normal(glm::vec3 const* v) noexcept {
    glm::vec3 ab = v[1] - v[0];
    glm::vec3 ac = v[2] - v[0];
    glm::vec3 perpendiular = glm::cross(ab, ac);
    return glm::normalize(perpendiular);
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

glm::mat4 osc::rot_dir1_to_dir2(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    float cos_ang = glm::dot(a, b);

    if (cos_ang > 0.999f) {
        return glm::mat4{1.0f}; // angle too small to compute cross product
    }

    glm::vec3 rotation_axis = glm::cross(a, b);
    float angle = glm::acos(cos_ang);
    return glm::rotate(glm::mat4{1.0f}, angle, rotation_axis);
}

std::ostream& osc::operator<<(std::ostream& o, AABB const& aabb) {
    return o << "AABB(min = " << aabb.min << ", max = " << aabb.max << ')';
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
    for (int i = 0; i < 3; ++i) {
        if (a.min[i] == a.max[i]) {
            return true;
        }
    }
    return false;
}

glm::vec3::length_type osc::aabb_longest_dim_idx(AABB const& a) noexcept {
    return vec_longest_dim_idx(aabb_dims(a));
}

std::array<glm::vec3, 8> osc::aabb_verts(AABB const& aabb) noexcept {

    glm::vec3 d = aabb_dims(aabb);

    std::array<glm::vec3, 8> rv;
    rv[0] = aabb.min;
    rv[1] = aabb.max;
    int pos = 2;
    for (int i = 0; i < 3; ++i) {
        glm::vec3 min = aabb.min;
        min[i] += d[i];
        glm::vec3 max = aabb.max;
        max[i] -= d[i];
        rv[pos++] = min;
        rv[pos++] = max;
    }
    return rv;
}

AABB osc::aabb_apply_xform(AABB const& aabb, glm::mat4 const& m) noexcept {
    auto verts = aabb_verts(aabb);
    for (auto& vert : verts) {
        vert = m * glm::vec4{vert, 1.0f};
    }

    return aabb_from_points(verts.data(), verts.size());
}

AABB osc::aabb_from_points(glm::vec3 const* vs, size_t n) noexcept {
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
        glm::vec3 const& pos = vs[i];
        rv.min = vec_min(rv.min, pos);
        rv.max = vec_max(rv.max, pos);
    }

    return rv;
}

std::ostream& osc::operator<<(std::ostream& o, Sphere const& s) {
    return o << "Sphere(origin = " << s.origin << ", radius = " << s.radius << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Line const& l) {
    return o << "Line(origin = " << l.o << ", direction = " << l.d << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Plane const& p) {
    return o << "Plane(origin = " << p.origin << ", normal = " << p.normal << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Disc const& d) {
    return o << "Disc(origin = " << d.origin << ", normal = " << d.normal << ", radius = " << d.radius << ')';
}

Sphere osc::sphere_bounds_of_points(glm::vec3 const* vs, size_t n) noexcept {
    AABB aabb = aabb_from_points(vs, n);

    Sphere rv{};
    rv.origin = (aabb.min + aabb.max) / 2.0f;
    rv.radius = 0.0f;

    // edge-case: no points provided
    if (n == 0) {
        return rv;
    }

    float biggest_r2 = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        glm::vec3 const& pos = vs[i];
        glm::vec3 pos2rv = pos - rv.origin;
        float r2 = glm::dot(pos2rv, pos2rv);
        biggest_r2 = std::max(biggest_r2, r2);
    }

    rv.radius = glm::sqrt(biggest_r2);

    return rv;
}

AABB osc::sphere_aabb(Sphere const& s) noexcept {
    AABB rv;
    rv.min = s.origin - s.radius;
    rv.max = s.origin + s.radius;
    return rv;
}

Line osc::apply_xform_to_line(Line const& l, glm::mat4 const& m) noexcept {
    Line rv;
    rv.d = m * glm::vec4{l.d, 0.0f};
    rv.o = m * glm::vec4{l.o, 1.0f};
    return rv;
}

glm::mat4 osc::disc_to_disc_xform(Disc const& a, Disc const& b) noexcept {
    // this is essentially LERPing [0,1] onto [1, l] to rescale only
    // along the line's original direction

    // scale factor
    float s = b.radius/a.radius;

    // LERP the axes as follows
    //
    // - 1.0f if parallel with N
    // - s if perpendicular to N
    // - N is a directional vector, so it's `cos(theta)` in each axis already
    // - 1-N is sin(theta) of each axis to the normal
    // - LERP is 1.0f + (s - 1.0f)*V, where V is how perpendiular each axis is

    glm::vec3 scalers = 1.0f + ((s - 1.0f) * glm::abs(1.0f - a.normal));
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, scalers);

    float cos_theta = glm::dot(a.normal, b.normal);
    glm::mat4 rotator;
    if (cos_theta > 0.9999f) {
        rotator = glm::mat4{1.0f};
    } else {
        float theta = glm::acos(cos_theta);
        glm::vec3 axis = glm::cross(a.normal, b.normal);
        rotator = glm::rotate(glm::mat4{1.0f}, theta, axis);
    }

    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, b.origin-a.origin);

    return translator * rotator * scaler;
}

glm::mat4 osc::sphere_to_sphere_xform(Sphere const& a, Sphere const& b) noexcept {
    float scale = b.radius/a.radius;
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, glm::vec3{scale, scale, scale});
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, b.origin - a.origin);
    return mover * scaler;
}

glm::mat4 osc::segment_to_segment_xform(Segment const& a, Segment const& b) noexcept {
    glm::vec3 a1_to_a2 = a.p2 - a.p1;
    glm::vec3 b1_to_b2 = b.p2 - b.p1;

    float a_len = glm::length(a1_to_a2);
    float b_len = glm::length(b1_to_b2);

    glm::vec3 a_dir = a1_to_a2/a_len;
    glm::vec3 b_dir = b1_to_b2/b_len;

    glm::vec3 a_center = (a.p1 + a.p2)/2.0f;
    glm::vec3 b_center = (b.p1 + b.p2)/2.0f;

    // this is essentially LERPing [0,1] onto [1, l] to rescale only
    // along the line's original direction
    float s = b_len/a_len;
    glm::vec3 scaler = glm::vec3{1.0f, 1.0f, 1.0f} + (s-1.0f)*a_dir;

    glm::mat4 rotate = rot_dir1_to_dir2(a_dir, b_dir);
    glm::mat4 scale = glm::scale(glm::mat4{1.0f}, scaler);
    glm::mat4 move = glm::translate(glm::mat4{1.0f}, b_center - a_center);

    return move * rotate * scale;
}


Ray_collision osc::get_ray_collision_sphere(Line const& l, Sphere const& s) noexcept {
    return ray_collision_sphere_analytic(s, l);
}

Ray_collision osc::get_ray_collision_AABB(Line const& l, AABB const& bb) noexcept {
    Ray_collision rv;

    float t0 = std::numeric_limits<float>::lowest();
    float t1 = std::numeric_limits<float>::max();

    // intersect the ray with each axis-aligned slab for each dimension
    //
    // i.e. figure out where the line intersects the front+back of the AABB
    //      in (e.g.) X, then Y, then Z, and intersect those interactions such
    //      that if the intersection is ever empty (or, negative here) then there
    //      is no intersection
    for (int i = 0; i < 3; ++i) {
        float invDir = 1.0f / l.d[i];
        float tNear = (bb.min[i] - l.o[i]) * invDir;
        float tFar = (bb.max[i] - l.o[i]) * invDir;
        if (tNear > tFar) {
            std::swap(tNear, tFar);
        }
        t0 = std::max(t0, tNear);
        t1 = std::min(t1, tFar);

        if (t0 > t1) {
            rv.hit = false;
            return rv;
        }
    }

    rv.hit = true;
    rv.distance = t0;  // other == t1
    return rv;
}

Ray_collision osc::get_ray_collision_plane(Line const& l, Plane const& p) noexcept {
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

    Ray_collision rv;

    float denominator = glm::dot(p.normal, l.d);

    if (std::abs(denominator) > 1e-6) {
        float numerator = glm::dot(p.origin - l.o, p.normal);
        rv.hit = true;
        rv.distance = numerator / denominator;
        return rv;
    } else {
        // the line is *very* parallel to the plane, which could cause
        // some divide-by-zero havok: pretend it didn't intersect
        rv.hit = false;
        return rv;
    }
}

Ray_collision osc::get_ray_collision_disc(Line const& l, Disc const& d) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection

    // think of this as a ray-plane intersection test with the additional
    // constraint that the ray has to be within the radius of the disc

    Ray_collision rv;

    Plane p;
    p.origin = d.origin;
    p.normal = d.normal;

    auto [hits_plane, t] = get_ray_collision_plane(l, p);

    if (!hits_plane) {
        rv.hit = false;
        return rv;
    }

    // figure out whether the plane hit is within the disc's radius
    glm::vec3 pos = l.o + t*l.d;
    glm::vec3 v = pos - d.origin;
    float d2 = glm::dot(v, v);
    float r2 = glm::dot(d.radius, d.radius);

    if (d2 > r2) {
        rv.hit = false;
        return rv;
    }

    rv.hit = true;
    rv.distance = t;
    return rv;
}

Ray_collision osc::get_ray_collision_triangle(Line const& l, glm::vec3 const* v) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution

    Ray_collision rv;

    // compute triangle normal
    glm::vec3 N = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));

    // compute dot product between normal and ray
    float NdotR = glm::dot(N, l.d);

    // if the dot product is small, then the ray is probably very parallel to
    // the triangle (or, perpendicular to the normal) and doesn't intersect
    if (std::abs(NdotR) < std::numeric_limits<float>::epsilon()) {
        rv.hit = false;
        return rv;
    }

    // - v[0] is a known point on the plane
    // - N is a normal to the plane
    // - N.v[0] is the projection of v[0] onto N and indicates how long along N to go to hit some
    //   other point on the plane
    float D = glm::dot(N, v[0]);

    // ok, that's one side of the equation
    //
    // - the other side of the equation is that the same is true for *any* point on the plane
    // - so: D = P.N also
    // - where P == O + tR (our line)
    // - expand: D = (O + tR).N
    // - rearrange:
    //
    //     D = O.N + t.R.N
    //     D - O.N = t.R.N
    //     (D - O.N)/(R.N) = t
    //
    // tah-dah: we have the ray distance
    float t = -(glm::dot(N, l.o) - D) / NdotR;

    // if triangle plane is behind line then return early
    if (t < 0.0f) {
        rv.hit = false;
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
            rv.hit = false;
            return rv;
        }
    }

    rv.hit = true;
    rv.distance = t;
    return rv;
}

Rgba32 osc::rgba32_from_vec4(glm::vec4 const& v) noexcept {
    Rgba32 rv;
    rv.r = static_cast<unsigned char>(255.0f * v.r);
    rv.g = static_cast<unsigned char>(255.0f * v.g);
    rv.b = static_cast<unsigned char>(255.0f * v.b);
    rv.a = static_cast<unsigned char>(255.0f * v.a);
    return rv;
}

Rgba32 osc::rgba32_from_f4(float r, float g, float b, float a) noexcept {
    Rgba32 rv;
    rv.r = static_cast<unsigned char>(255.0f * r);
    rv.g = static_cast<unsigned char>(255.0f * g);
    rv.b = static_cast<unsigned char>(255.0f * b);
    rv.a = static_cast<unsigned char>(255.0f * a);
    return rv;
}

Rgba32 osc::rgba32_from_u32(uint32_t v) noexcept {
    Rgba32 rv;
    rv.r = static_cast<unsigned char>((v >> 24) & 0xff);
    rv.g = static_cast<unsigned char>((v >> 16) & 0xff);
    rv.b = static_cast<unsigned char>((v >> 8) & 0xff);
    rv.a = static_cast<unsigned char>((v >> 0) & 0xff);
    return rv;
}

void osc::NewMesh::clear() {
    verts.clear();
    normals.clear();
    texcoords.clear();
    indices.clear();
}

void osc::NewMesh::reserve(size_t n) {
    verts.reserve(n);
    normals.reserve(n);
    texcoords.reserve(n);
    indices.reserve(n);
}

std::ostream& osc::operator<<(std::ostream& o, NewMesh const& m) {
    return o << "Mesh(nverts = " << m.verts.size() << ", nnormals = " << m.normals.size() << ", ntexcoords = " << m.texcoords.size() << ", nindices = " << m.indices.size() << ')';
}

NewMesh osc::gen_textured_quad() {
    NewMesh rv;
    rv.reserve(g_ShadedTexturedQuadVerts.size());

    unsigned short index = 0;
    for (Textured_vert const& v : g_ShadedTexturedQuadVerts) {
        rv.verts.push_back(v.pos);
        rv.normals.push_back(v.norm);
        rv.texcoords.push_back(v.uv);
        rv.indices.push_back(index++);
    }

    return rv;
}

NewMesh osc::gen_untextured_uv_sphere(size_t sectors, size_t stacks) {
    NewMesh rv;
    rv.reserve(2*3*stacks*sectors);

    // this is a shitty alg that produces a shitty UV sphere. I don't have
    // enough time to implement something better, like an isosphere, or
    // something like a patched sphere:
    //
    // https://www.iquilezles.org/www/articles/patchedsphere/patchedsphere.htm
    //
    // This one is adapted from:
    //    http://www.songho.ca/opengl/gl_sphere.html#example_cubesphere


    // polar coords, with [0, 0, -1] pointing towards the screen with polar
    // coords theta = 0, phi = 0. The coordinate [0, 1, 0] is theta = (any)
    // phi = PI/2. The coordinate [1, 0, 0] is theta = PI/2, phi = 0
    std::vector<Untextured_vert> points;

    float theta_step = 2.0f * fpi / sectors;
    float phi_step = fpi / stacks;

    for (size_t stack = 0; stack <= stacks; ++stack) {
        float phi = fpi2 - static_cast<float>(stack) * phi_step;
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

    unsigned short index = 0;
    auto push = [&rv, &index](Untextured_vert const& v) {
        rv.verts.push_back(v.pos);
        rv.normals.push_back(v.norm);
        rv.indices.push_back(index++);
    };

    for (size_t stack = 0; stack < stacks; ++stack) {
        size_t k1 = stack * (sectors + 1);
        size_t k2 = k1 + sectors + 1;

        for (size_t sector = 0; sector < sectors; ++sector, ++k1, ++k2) {
            // 2 triangles per sector - excluding the first and last stacks
            // (which contain one triangle, at the poles)

            Untextured_vert const& p1 = points[k1];
            Untextured_vert const& p2 = points[k2];
            Untextured_vert const& p1_plus1 = points[k1+1];
            Untextured_vert const& p2_plus1 = points[k2+1];

            if (stack != 0) {
                push(p1);
                push(p1_plus1);
                push(p2);
            }

            if (stack != (stacks - 1)) {
                push(p1_plus1);
                push(p2_plus1);
                push(p2);
            }
        }
    }

    return rv;
}

NewMesh osc::gen_untextured_simbody_cylinder(size_t nsides) {
    NewMesh rv;
    rv.reserve(4*3*nsides);

    constexpr float top_y = +1.0f;
    constexpr float bottom_y = -1.0f;
    const float step_angle = 2.0f*fpi / nsides;

    unsigned short index = 0;
    auto push = [&rv, &index](glm::vec3 const& pos, glm::vec3 const& norm) {
        rv.verts.push_back(pos);
        rv.normals.push_back(norm);
        rv.indices.push_back(index++);
    };

    // top
    {
        glm::vec3 normal = {0.0f, 1.0f, 0.0f};
        glm::vec3 middle = {0.0f, top_y, 0.0f};

        for (size_t i = 0; i < nsides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            // note: ensure these wind CCW (culling)
            glm::vec3 e1 = {cosf(theta_end), top_y, sinf(theta_end)};
            glm::vec3 e2 = {cosf(theta_start), top_y, sinf(theta_start)};

            push(middle, normal);
            push(e1, normal);
            push(e2, normal);
        }
    }

    // bottom
    {
        glm::vec3 normal = {0.0f, -1.0f, 0.0f};
        glm::vec3 middle = {0.0f, bottom_y, 0.0f};

        for (size_t i = 0; i < nsides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            // note: ensure these wind CCW (culling)
            glm::vec3 e1 = {cosf(theta_start), bottom_y, sinf(theta_start)};
            glm::vec3 e2 = {cosf(theta_end), bottom_y, sinf(theta_end)};

            push(middle, normal);
            push(e1, normal);
            push(e2, normal);
        }
    }

    // sides
    for (size_t i = 0; i < nsides; ++i) {
        float theta_start = i * step_angle;
        float theta_end = theta_start + step_angle;
        float norm_theta = theta_start + (step_angle / 2.0f);

        glm::vec3 normal = {cosf(norm_theta), 0.0f, sinf(norm_theta)};
        glm::vec3 top1 = {cosf(theta_start), top_y, sinf(theta_start)};
        glm::vec3 top2 = {cosf(theta_end), top_y, sinf(theta_end)};
        glm::vec3 bottom1 = {top1.x, bottom_y, top1.z};
        glm::vec3 bottom2 = {top2.x, bottom_y, top2.z};

        // draw quads CCW for each side

        push(top1, normal);
        push(top2, normal);
        push(bottom1, normal);

        push(bottom2, normal);
        push(bottom1, normal);
        push(top2, normal);
    }

    return rv;
}

NewMesh osc::gen_untextured_simbody_cone(size_t nsides) {
    NewMesh rv;
    rv.reserve(2*3*nsides);

    constexpr float top_y = +1.0f;
    constexpr float bottom_y = -1.0f;
    const float step_angle = 2.0f*fpi / nsides;

    unsigned short index = 0;
    auto push = [&rv, &index](glm::vec3 const& pos, glm::vec3 const& norm) {
        rv.verts.push_back(pos);
        rv.normals.push_back(norm);
        rv.indices.push_back(index++);
    };

    // bottom
    {
        glm::vec3 normal = {0.0f, -1.0f, 0.0f};
        glm::vec3 middle = {0.0f, bottom_y, 0.0f};

        for (size_t i = 0; i < nsides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            glm::vec3 p1 = {cosf(theta_start), bottom_y, sinf(theta_start)};
            glm::vec3 p2 = {cosf(theta_end), bottom_y, sinf(theta_end)};

            push(middle, normal);
            push(p1, normal);
            push(p2, normal);
        }
    }

    // sides
    {
        for (size_t i = 0; i < nsides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            glm::vec3 points[3] = {
                {0.0f, top_y, 0.0f},
                {cosf(theta_end), bottom_y, sinf(theta_end)},
                {cosf(theta_start), bottom_y, sinf(theta_start)},                
            };

            glm::vec3 normal = triangle_normal(points);

            push(points[0], normal);
            push(points[1], normal);
            push(points[2], normal);
        }
    }

    return rv;
}

NewMesh osc::gen_NxN_grid(size_t n) {
    static constexpr float z = 0.0f;
    static constexpr float min = -1.0f;
    static constexpr float max = 1.0f;

    float step_size = (max - min) / static_cast<float>(n);

    size_t nlines = n + 1;

    NewMesh rv;
    rv.verts.reserve(4*nlines);
    rv.indices.reserve(4*nlines);

    unsigned short index = 0;
    auto push = [&index, &rv](glm::vec3 const& pos) {
        rv.verts.push_back(pos);
        rv.indices.push_back(index++);
    };

    // lines parallel to X axis
    for (size_t i = 0; i < nlines; ++i) {
        float y = min + i * step_size;

        push({-1.0f, y, z});
        push({+1.0f, y, z});
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < nlines; ++i) {
        float x = min + i * step_size;

        push({x, -1.0f, z});
        push({x, +1.0f, z});
    }

    return rv;
}

NewMesh osc::gen_y_line() {
    NewMesh rv;
    rv.verts = {{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    rv.indices = {0, 1};
    return rv;
}

NewMesh osc::gen_cube() {
    NewMesh rv;
    rv.reserve(g_ShadedTexturedCubeVerts.size());

    unsigned short index = 0;
    for (auto const& v : g_ShadedTexturedCubeVerts) {
        rv.verts.push_back(v.pos);
        rv.normals.push_back(v.norm);
        rv.texcoords.push_back(v.uv);
        rv.indices.push_back(index++);
    }

    return rv;
}

NewMesh osc::gen_cube_lines() {
    NewMesh rv;
    rv.verts.reserve(g_CubeEdgeLines.size());
    rv.indices.reserve(g_CubeEdgeLines.size());

    unsigned short index = 0;
    for (auto const& v : g_CubeEdgeLines) {
        rv.verts.push_back(v.pos);
        rv.indices.push_back(index++);
    }

    return rv;
}

NewMesh osc::gen_circle(size_t nsides) {
    NewMesh rv;
    rv.verts.reserve(3*nsides);

    unsigned short index = 0;
    auto push = [&rv, &index](float x, float y, float z) {
        rv.verts.push_back({x, y, z});
        rv.indices.push_back(index++);
    };

    float step = 2.0f*fpi / static_cast<float>(nsides);
    for (size_t i = 0; i < nsides; ++i) {
        float theta1 = i * step;
        float theta2 = (i+1) * step;

        push(0.0f, 0.0f, 0.0f);
        push(sinf(theta1), cosf(theta1), 0.0f);
        push(sinf(theta2), cosf(theta2), 0.0f);
    }

    return rv;
}

glm::vec2 osc::topleft_relpos_to_ndcxy(glm::vec2 relpos) {
    relpos.y = 1.0f - relpos.y;
    return 2.0f*relpos - 1.0f;
}

// converts a topleft-origin RELATIVE `pos` (0 to 1 in XY, starting topleft) into
// the equivalent POINT on the front of the NDC cube (i.e. "as if" a viewer was there)
//
// i.e. {X_ndc, Y_ndc, -1.0f, 1.0f}
glm::vec4 osc::topleft_relpos_to_ndc_cube(glm::vec2 relpos) {
    return {topleft_relpos_to_ndcxy(relpos), -1.0f, 1.0f};
}

osc::Polar_perspective_camera::Polar_perspective_camera() :
    radius{1.0f},
    theta{0.0f},
    phi{0.0f},
    pan{0.0f, 0.0f, 0.0f},
    fov{120.0f},
    znear{0.1f},
    zfar{100.0f} {

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
    theta += 2.0f * fpi * -delta.x;
    phi += 2.0f * fpi * delta.y;
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

Line osc::Polar_perspective_camera::screenpos_to_world_ray(glm::vec2 pos, glm::vec2 dims) const noexcept {
    glm::mat4 proj_mtx = projection_matrix(dims.x/dims.y);
    glm::mat4 view_mtx = view_matrix();

    // position of point, as if it were on the front of the 3D NDC cube
    glm::vec4 line_origin_ndc = topleft_relpos_to_ndc_cube(pos/dims);

    glm::vec4 line_origin_view = glm::inverse(proj_mtx) * line_origin_ndc;
    line_origin_view /= line_origin_view.w;  // perspective divide

    // location of mouse in worldspace
    glm::vec3 line_origin_world = glm::vec3{glm::inverse(view_mtx) * line_origin_view};

    // direction vector from camera to mouse location (i.e. the projection)
    glm::vec3 line_dir_world = glm::normalize(line_origin_world - this->pos());

    Line rv;
    rv.d = line_dir_world;
    rv.o = line_origin_world;
    return rv;
}

osc::Euler_perspective_camera::Euler_perspective_camera() :
    pos{0.0f, 0.0f, 0.0f},
    pitch{0.0f},
    yaw{-fpi/2.0f},
    fov{fpi * 70.0f/180.0f},
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

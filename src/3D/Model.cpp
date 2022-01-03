#include "src/3D/Model.hpp"

#include "src/3D/Constants.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <array>
#include <iostream>
#include <cstddef>
#include <limits>

using namespace osc;

namespace {
    struct UntexturedVert {
        glm::vec3 pos;
        glm::vec3 norm;
    };

    struct TexturedVert {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 uv;
    };
}

// standard textured cube with dimensions [-1, +1] in xyz and uv coords of
// (0, 0) bottom-left, (1, 1) top-right for each (quad) face
static constexpr std::array<TexturedVert, 36> g_ShadedTexturedCubeVerts = {{

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
static constexpr std::array<TexturedVert, 6> g_ShadedTexturedQuadVerts = {{

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
static constexpr std::array<UntexturedVert, 24> g_CubeEdgeLines = {{
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

struct QuadraticFormulaResult final {
    bool computeable;
    float x0;
    float x1;
};

// solve a quadratic formula
//
// only real-valued results supported - no complex-plane results
static QuadraticFormulaResult solveQuadratic(float a, float b, float c) {
    QuadraticFormulaResult res;

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


static RayCollision GetRayCollisionSphere(Sphere const& s, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    RayCollision rv;

    glm::vec3 L = s.origin - l.origin;  // line origin to sphere origin
    float tca = glm::dot(L, l.dir);  // projected line from middle of hitline to sphere origin

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

static RayCollision GetRayCollisionSphereAnalytic(Sphere const& s, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    RayCollision rv;

    glm::vec3 L = l.origin - s.origin;

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

    float a = glm::dot(l.dir, l.dir);  // always == 1.0f if d is normalized
    float b = 2.0f * glm::dot(l.dir, L);
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

std::ostream& osc::operator<<(std::ostream& o, glm::quat const& q)
{
    return o << "quat(x = " << q.x << ", y = " << q.y << ", z = " << q.z << ", w = " << q.w << ')';
}

bool osc::AreAtSameLocation(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    float eps = std::numeric_limits<float>::epsilon();
    float eps2 = eps * eps;
    glm::vec3 b2a = a - b;
    float len2 = glm::dot(b2a, b2a);
    return len2 > eps2;
}

glm::vec3 osc::VecMin(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    return glm::vec3{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

glm::vec2 osc::VecMin(glm::vec2 const& a, glm::vec2 const& b) noexcept {
    return glm::vec2{std::min(a.x, b.x), std::min(a.y, b.y)};
}

glm::vec3 osc::VecMax(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    return glm::vec3{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

glm::vec2 osc::VecMax(glm::vec2 const& a, glm::vec2 const& b) noexcept {
    return glm::vec2{std::max(a.x, b.x), std::max(a.y, b.y)};
}

glm::vec3::length_type osc::VecLongestDimIdx(glm::vec3 const& v) noexcept {
    if (v.x > v.y && v.x > v.z) {
        return 0;  // X is longest
    } else if (v.y > v.z) {
        return 1;  // Y is longest
    } else {
        return 2;  // Z is longest
    }
}

glm::vec2::length_type osc::VecLongestDimIdx(glm::vec2 v) noexcept {
    if (v.x > v.y) {
        return 0;  // X is longest
    } else {
        return 1;
    }
}

glm::ivec2::length_type osc::VecLongestDimIdx(glm::ivec2 v) noexcept {
    if (v.x > v.y) {
        return 0;  // X is longest
    } else {
        return 1;
    }
}

float osc::VecLongestDimVal(glm::vec3 const& v) noexcept {
    return v[VecLongestDimIdx(v)];
}

float osc::VecLongestDimVal(glm::vec2 v) noexcept {
    return v[VecLongestDimIdx(v)];
}

int osc::VecLongestDimVal(glm::ivec2 v) noexcept {
    return v[VecLongestDimIdx(v)];
}

float osc::VecAspectRatio(glm::ivec2 v) noexcept {
    return static_cast<float>(v.x) / static_cast<float>(v.y);
}

float osc::VecAspectRatio(glm::vec2 v) noexcept {
    return v.x/v.y;
}

glm::vec3 osc::VecKahanSum(glm::vec3 const* vs, size_t n) noexcept
{
    glm::vec3 sum{};  // accumulator
    glm::vec3 c{};    // running compensation of low-order bits

    for (size_t i = 0; i < n; ++i) {
        glm::vec3 y = vs[i] - c;  // subtract the compensation amount from the next number
        glm::vec3 t = sum + y;    // perform the summation (might lose information)
        c = (t - sum) - y;        // (t-sum) yields the retained (high-order) parts of `y`, so `c` contains the "lost" information
        sum = t;                  // CAREFUL: algebreically, `c` always == 0 - despite the computer's (actual) limited precision, the compiler might elilde all of this
    }

    return sum;
}

glm::vec3 osc::VecNumericallyStableAverage(glm::vec3 const* vs, size_t n) noexcept
{
    glm::vec3 sum = VecKahanSum(vs, n);
    return sum / static_cast<float>(n);
}

glm::vec3 osc::TriangleNormal(glm::vec3 const* v) noexcept {
    glm::vec3 ab = v[1] - v[0];
    glm::vec3 ac = v[2] - v[0];
    glm::vec3 perpendiular = glm::cross(ab, ac);
    return glm::normalize(perpendiular);
}

glm::vec3 osc::TriangleNormal(glm::vec3 const& a, glm::vec3 const& b, glm::vec3 const& c) noexcept {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 perpendiular = glm::cross(ab, ac);
    return glm::normalize(perpendiular);
}

glm::mat3 osc::NormalMatrix(glm::mat4 const& m) noexcept {
    glm::mat3 topLeft{m};
    return glm::inverse(glm::transpose(topLeft));
}

glm::mat3 osc::NormalMatrix(glm::mat4x3 const& m) noexcept {
    glm::mat3 topLeft{m};
    return glm::inverse(glm::transpose(topLeft));
}

glm::mat4 osc::Dir1ToDir2Xform(glm::vec3 const& a, glm::vec3 const& b) noexcept {
    float cosAng = glm::dot(a, b);

    if (std::fabs(cosAng) > 0.999f) {
        // the vectors can't form a parallelogram, so the cross product is going
        // to be zero
        //
        // "More generally, the magnitude of the product equals the area of a parallelogram
        //  with the vectors for sides" - https://en.wikipedia.org/wiki/Cross_product
        return glm::mat4{1.0f};
    }

    glm::vec3 rotAxis = glm::cross(a, b);
    float angle = glm::acos(cosAng);
    return glm::rotate(glm::mat4{1.0f}, angle, rotAxis);
}

glm::vec3 osc::extractEulerAngleXYZ(glm::mat4 const& m) noexcept
{
    glm::vec3 v;
    glm::extractEulerAngleXYZ(m, v.x, v.y, v.z);
    return v;
}

// Transform impl.

Transform osc::Transform::atPosition(glm::vec3 const& pos) noexcept
{
    Transform rv;
    rv.position = pos;
    return rv;
}

Transform osc::Transform::byDecomposing(glm::mat4 const& mtx)
{
    Transform rv;
    glm::vec3 skew;
    glm::vec4 perspective;
    if (!glm::decompose(mtx, rv.scale, rv.rotation, rv.position, skew, perspective)) {
        throw std::runtime_error{"failed to decompose a matrix into scale, rotation, etc."};
    }
    return rv;
}

osc::Transform::Transform() noexcept :
    position{0.0f, 0.0f, 0.0f},
    rotation{1.0f, 0.0f, 0.0f, 0.0f},
    scale{1.0f, 1.0f, 1.0f}
{
}

osc::Transform::Transform(glm::vec3 const& position_) noexcept :
    position{std::move(position_)},
    rotation{1.0f, 0.0f, 0.0f, 0.0f},
    scale{1.0f, 1.0f, 1.0f}
{
}

osc::Transform::Transform(glm::vec3 const& position_, glm::quat const& rotation_) noexcept :
    position{std::move(position_)},
    rotation{std::move(rotation_)},
    scale{1.0f, 1.0f, 1.0f}
{
}

osc::Transform::Transform(glm::vec3 const& position_,
                          glm::quat const& rotation_,
                          glm::vec3 const& scale_) noexcept :
    position{position_},
    rotation{rotation_},
    scale{scale_}
{
}

Transform osc::Transform::withPosition(glm::vec3 const& pos) const noexcept
{
    Transform t{*this};
    t.position = pos;
    return t;
}

Transform osc::Transform::withRotation(glm::quat const& rot) const noexcept
{
    Transform t{*this};
    t.rotation = rot;
    return t;
}

Transform osc::Transform::withScale(glm::vec3 const& s) const noexcept
{
    Transform t{*this};
    t.scale = s;
    return t;
}

std::ostream& osc::operator<<(std::ostream& o, Transform const& t)
{
    using osc::operator<<;
    return o << "Transform(position = " << t.position << ", rotation = " << t.rotation << ", scale = " << t.scale << ')';
}

Transform& osc::operator+=(Transform& t, Transform const& o) noexcept
{
    t.position += o.position;
    t.rotation += o.rotation;
    t.scale += o.scale;
    return t;
}

Transform& osc::operator/=(Transform& t, float s) noexcept
{
    t.position /= s;
    t.rotation /= s;
    t.scale /= s;
    return t;
}

glm::mat4 osc::toMat4(Transform const& t) noexcept
{
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, t.scale);
    glm::mat4 rotater = glm::toMat4(t.rotation);
    glm::mat4 translater = glm::translate(glm::mat4{1.0f}, t.position);

    return translater * rotater * scaler;
}

glm::mat4 osc::toInverseMat4(Transform const& t) noexcept
{
    glm::mat4 translater = glm::translate(glm::mat4{1.0f}, -t.position);
    glm::mat4 rotater = glm::toMat4(glm::conjugate(t.rotation));
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, 1.0f/t.scale);

    return scaler * rotater * translater;
}

glm::mat3x3 osc::toNormalMatrix(Transform const& t) noexcept
{
    return NormalMatrix(toMat4(t));
}

glm::vec3 osc::transformDirection(Transform const& t, glm::vec3 const& localDir) noexcept
{
    return t.rotation * localDir;
}

glm::vec3 osc::inverseTransformDirection(Transform const& t, glm::vec3 const& worldDir) noexcept
{
    return glm::conjugate(t.rotation) * worldDir;
}

glm::vec3 osc::transformPoint(Transform const& t, glm::vec3 const& localPoint) noexcept
{
    glm::vec3 rv = localPoint;
    rv *= t.scale;
    rv = t.rotation * rv;
    rv += t.position;
    return rv;
}

glm::vec3 osc::inverseTransformPoint(Transform const& t, glm::vec3 const& worldPoint) noexcept
{
    glm::vec3 rv = worldPoint;
    rv -= t.position;
    rv = glm::conjugate(t.rotation) * rv;
    rv /= t.scale;
    return rv;
}

void osc::applyWorldspaceRotation(Transform& t,
                                  glm::vec3 const& eulerAngles,
                                  glm::vec3 const& rotationCenter) noexcept
{
    glm::quat q{eulerAngles};
    t.position = q*(t.position - rotationCenter) + rotationCenter;
    t.rotation = glm::normalize(q*t.rotation);
}

glm::vec3 osc::eulerAnglesXYZ(Transform const& t) noexcept
{
    glm::vec3 rv;
    glm::extractEulerAngleXYZ(glm::toMat4(t.rotation), rv.x, rv.y, rv.z);
    return rv;
}

glm::vec3 osc::eulerAnglesExtrinsic(Transform const& t) noexcept
{
    return glm::eulerAngles(t.rotation);
}

std::ostream& osc::operator<<(std::ostream& o, Rect const& r) {
    return o << "Rect(p1 = " << r.p1 << ", p2 = " << r.p2 << ")";
}

glm::vec2 osc::RectDims(Rect const& r) noexcept {
    return glm::abs(r.p2 - r.p1);
}

float osc::RectAspectRatio(Rect const& r) noexcept {
    glm::vec2 dims = RectDims(r);
    return dims.x/dims.y;
}

glm::mat4 osc::GroundToSphereXform(Sphere const& s) noexcept {
    return glm::translate(glm::mat4{1.0f}, s.origin) * glm::scale(glm::mat4{1.0f}, {s.radius, s.radius, s.radius});
}

bool osc::PointIsInRect(Rect const& r, glm::vec2 const& p) noexcept {
    glm::vec2 relPos = p - r.p1;
    glm::vec2 dims = RectDims(r);
    return (0.0f <= relPos.x && relPos.x <= dims.x) && (0.0f <= relPos.y && relPos.y <= dims.y);
}

std::ostream& osc::operator<<(std::ostream& o, AABB const& aabb) {
    return o << "AABB(min = " << aabb.min << ", max = " << aabb.max << ')';
}

glm::vec3 osc::AABBCenter(AABB const& a) noexcept {
    return (a.min + a.max)/2.0f;
}

glm::vec3 osc::AABBDims(AABB const& a) noexcept {
    return a.max - a.min;
}

AABB osc::AABBUnion(AABB const& a, AABB const& b) noexcept {
    return AABB{VecMin(a.min, b.min), VecMax(a.max, b.max)};
}

AABB osc::AABBUnion(void const* data, size_t n, size_t stride, size_t offset) {
    if (n == 0) {
        return AABB{};
    }

    assert(reinterpret_cast<uintptr_t>(data) % alignof(AABB) == 0 && "possible unaligned load detected: this will cause bugs on systems that only support aligned loads (e.g. ARM)");
    assert(reinterpret_cast<uintptr_t>(offset) % alignof(AABB) == 0 && "possible unaligned load detected: this will cause bugs on systems that only support aligned loads (e.g. ARM)");

    unsigned char const* firstPtr = reinterpret_cast<unsigned char const*>(data);

    AABB rv = *reinterpret_cast<AABB const*>(firstPtr + offset);
    for (size_t i = 1; i < n; ++i) {
        unsigned char const* elPtr = firstPtr + (i*stride) + offset;
        AABB const& aabb = *reinterpret_cast<AABB const*>(elPtr);
        rv = AABBUnion(rv, aabb);
    }
    return rv;
}

bool osc::AABBIsEmpty(AABB const& a) noexcept {
    for (int i = 0; i < 3; ++i) {
        if (a.min[i] == a.max[i]) {
            return true;
        }
    }
    return false;
}

glm::vec3::length_type osc::AABBLongestDimIdx(AABB const& a) noexcept {
    return VecLongestDimIdx(AABBDims(a));
}

float osc::AABBLongestDim(AABB const& a) noexcept {
    glm::vec3 dims = AABBDims(a);
    float rv = dims[0];
    rv = std::max(rv, dims[1]);
    rv = std::max(rv, dims[2]);
    return rv;
}

std::array<glm::vec3, 8> osc::AABBVerts(AABB const& aabb) noexcept {

    glm::vec3 d = AABBDims(aabb);

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

AABB osc::AABBApplyXform(AABB const& aabb, glm::mat4 const& m) noexcept {
    auto verts = AABBVerts(aabb);
    for (auto& vert : verts) {
        glm::vec4 p = m * glm::vec4{vert, 1.0f};
        vert = glm::vec3{p / p.w}; // perspective divide
    }

    return AABBFromVerts(verts.data(), verts.size());
}

AABB osc::AABBFromVerts(glm::vec3 const* vs, size_t n) noexcept {
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
        rv.min = VecMin(rv.min, pos);
        rv.max = VecMax(rv.max, pos);
    }

    return rv;
}

std::ostream& osc::operator<<(std::ostream& o, Sphere const& s) {
    return o << "Sphere(origin = " << s.origin << ", radius = " << s.radius << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Line const& l) {
    return o << "Line(origin = " << l.origin << ", direction = " << l.dir << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Plane const& p) {
    return o << "Plane(origin = " << p.origin << ", normal = " << p.normal << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Disc const& d) {
    return o << "Disc(origin = " << d.origin << ", normal = " << d.normal << ", radius = " << d.radius << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Segment const& d) {
    return o << "Segment(p1 = " << d.p1 << ", p2 = " << d.p2 << ')';
}

Sphere osc::BoundingSphereFromVerts(glm::vec3 const* vs, size_t n) noexcept {
    AABB aabb = AABBFromVerts(vs, n);

    Sphere rv{};
    rv.origin = (aabb.min + aabb.max) / 2.0f;
    rv.radius = 0.0f;

    // edge-case: no points provided
    if (n == 0) {
        return rv;
    }

    float biggestR2 = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        glm::vec3 const& pos = vs[i];
        glm::vec3 pos2rv = pos - rv.origin;
        float r2 = glm::dot(pos2rv, pos2rv);
        biggestR2 = std::max(biggestR2, r2);
    }

    rv.radius = glm::sqrt(biggestR2);

    return rv;
}

AABB osc::SphereToAABB(Sphere const& s) noexcept {
    AABB rv;
    rv.min = s.origin - s.radius;
    rv.max = s.origin + s.radius;
    return rv;
}

Line osc::LineApplyXform(Line const& l, glm::mat4 const& m) noexcept {
    Line rv;
    rv.dir = m * glm::vec4{l.dir, 0.0f};
    rv.origin = m * glm::vec4{l.origin, 1.0f};
    return rv;
}

glm::mat4 osc::DiscToDiscXform(Disc const& a, Disc const& b) noexcept {
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

    float cosTheta = glm::dot(a.normal, b.normal);
    glm::mat4 rotator;
    if (cosTheta > 0.9999f) {
        rotator = glm::mat4{1.0f};
    } else {
        float theta = glm::acos(cosTheta);
        glm::vec3 axis = glm::cross(a.normal, b.normal);
        rotator = glm::rotate(glm::mat4{1.0f}, theta, axis);
    }

    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, b.origin-a.origin);

    return translator * rotator * scaler;
}

glm::mat4 osc::SphereToSphereXform(Sphere const& a, Sphere const& b) noexcept {
    float scale = b.radius/a.radius;
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, glm::vec3{scale, scale, scale});
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, b.origin - a.origin);
    return mover * scaler;
}

glm::mat4 osc::SegmentToSegmentXform(Segment const& a, Segment const& b) noexcept {
    glm::vec3 a1ToA2 = a.p2 - a.p1;
    glm::vec3 b1ToB2 = b.p2 - b.p1;

    float aLen = glm::length(a1ToA2);
    float bLen = glm::length(b1ToB2);

    glm::vec3 aDir = a1ToA2/aLen;
    glm::vec3 bDir = b1ToB2/bLen;

    glm::vec3 aCenter = (a.p1 + a.p2)/2.0f;
    glm::vec3 bCenter = (b.p1 + b.p2)/2.0f;

    // this is essentially LERPing [0,1] onto [1, l] to rescale only
    // along the line's original direction
    float s = bLen/aLen;
    glm::vec3 scaler = glm::vec3{1.0f, 1.0f, 1.0f} + (s-1.0f)*aDir;

    glm::mat4 rotate = Dir1ToDir2Xform(aDir, bDir);
    glm::mat4 scale = glm::scale(glm::mat4{1.0f}, scaler);
    glm::mat4 move = glm::translate(glm::mat4{1.0f}, bCenter - aCenter);

    return move * rotate * scale;
}


RayCollision osc::GetRayCollisionSphere(Line const& l, Sphere const& s) noexcept {
    return GetRayCollisionSphereAnalytic(s, l);
}

RayCollision osc::GetRayCollisionAABB(Line const& l, AABB const& bb) noexcept {
    RayCollision rv;

    float t0 = std::numeric_limits<float>::lowest();
    float t1 = std::numeric_limits<float>::max();

    // intersect the ray with each axis-aligned slab for each dimension
    //
    // i.e. figure out where the line intersects the front+back of the AABB
    //      in (e.g.) X, then Y, then Z, and intersect those interactions such
    //      that if the intersection is ever empty (or, negative here) then there
    //      is no intersection
    for (int i = 0; i < 3; ++i) {
        float invDir = 1.0f / l.dir[i];
        float tNear = (bb.min[i] - l.origin[i]) * invDir;
        float tFar = (bb.max[i] - l.origin[i]) * invDir;
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

RayCollision osc::GetRayCollisionPlane(Line const& l, Plane const& p) noexcept {
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

    RayCollision rv;

    float denominator = glm::dot(p.normal, l.dir);

    if (std::abs(denominator) > 1e-6) {
        float numerator = glm::dot(p.origin - l.origin, p.normal);
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

RayCollision osc::GetRayCollisionDisc(Line const& l, Disc const& d) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection

    // think of this as a ray-plane intersection test with the additional
    // constraint that the ray has to be within the radius of the disc

    RayCollision rv;

    Plane p;
    p.origin = d.origin;
    p.normal = d.normal;

    auto [hitsPlane, t] = GetRayCollisionPlane(l, p);

    if (!hitsPlane) {
        rv.hit = false;
        return rv;
    }

    // figure out whether the plane hit is within the disc's radius
    glm::vec3 pos = l.origin + t*l.dir;
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

RayCollision osc::GetRayCollisionTriangle(Line const& l, glm::vec3 const* v) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution

    RayCollision rv;

    // compute triangle normal
    glm::vec3 N = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));

    // compute dot product between normal and ray
    float NdotR = glm::dot(N, l.dir);

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
    float t = -(glm::dot(N, l.origin) - D) / NdotR;

    // if triangle plane is behind line then return early
    if (t < 0.0f) {
        rv.hit = false;
        return rv;
    }

    // intersection point on triangle plane, computed from line equation
    glm::vec3 P = l.origin + t*l.dir;

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

Rgba32 osc::Rgba32FromVec4(glm::vec4 const& v) noexcept {
    Rgba32 rv;
    rv.r = static_cast<unsigned char>(255.0f * v.r);
    rv.g = static_cast<unsigned char>(255.0f * v.g);
    rv.b = static_cast<unsigned char>(255.0f * v.b);
    rv.a = static_cast<unsigned char>(255.0f * v.a);
    return rv;
}

Rgba32 osc::Rgba32FromF4(float r, float g, float b, float a) noexcept {
    Rgba32 rv;
    rv.r = static_cast<unsigned char>(255.0f * r);
    rv.g = static_cast<unsigned char>(255.0f * g);
    rv.b = static_cast<unsigned char>(255.0f * b);
    rv.a = static_cast<unsigned char>(255.0f * a);
    return rv;
}

Rgba32 osc::Rgba32FromU32(uint32_t v) noexcept {
    Rgba32 rv;
    rv.r = static_cast<unsigned char>((v >> 24) & 0xff);
    rv.g = static_cast<unsigned char>((v >> 16) & 0xff);
    rv.b = static_cast<unsigned char>((v >> 8) & 0xff);
    rv.a = static_cast<unsigned char>((v >> 0) & 0xff);
    return rv;
}

void osc::MeshData::clear() {
    verts.clear();
    normals.clear();
    texcoords.clear();
    indices.clear();
}

void osc::MeshData::reserve(size_t n) {
    verts.reserve(n);
    normals.reserve(n);
    texcoords.reserve(n);
    indices.reserve(n);
}

std::ostream& osc::operator<<(std::ostream& o, MeshData const& m) {
    return o << "Mesh(nverts = " << m.verts.size() << ", nnormals = " << m.normals.size() << ", ntexcoords = " << m.texcoords.size() << ", nindices = " << m.indices.size() << ')';
}

MeshData osc::GenTexturedQuad() {
    MeshData rv;
    rv.reserve(g_ShadedTexturedQuadVerts.size());

    unsigned short index = 0;
    for (TexturedVert const& v : g_ShadedTexturedQuadVerts) {
        rv.verts.push_back(v.pos);
        rv.normals.push_back(v.norm);
        rv.texcoords.push_back(v.uv);
        rv.indices.push_back(index++);
    }

    return rv;
}

MeshData osc::GenUntexturedUVSphere(size_t sectors, size_t stacks) {
    MeshData rv;
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
    std::vector<UntexturedVert> points;

    float thetaStep = 2.0f * fpi / sectors;
    float phiStep = fpi / stacks;

    for (size_t stack = 0; stack <= stacks; ++stack) {
        float phi = fpi2 - static_cast<float>(stack) * phiStep;
        float y = sinf(phi);

        for (unsigned sector = 0; sector <= sectors; ++sector) {
            float theta = sector * thetaStep;
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
    auto push = [&rv, &index](UntexturedVert const& v) {
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

            UntexturedVert const& p1 = points[k1];
            UntexturedVert const& p2 = points[k2];
            UntexturedVert const& p1Plus1 = points[k1+1];
            UntexturedVert const& p2Plus1 = points[k2+1];

            if (stack != 0) {
                push(p1);
                push(p1Plus1);
                push(p2);
            }

            if (stack != (stacks - 1)) {
                push(p1Plus1);
                push(p2Plus1);
                push(p2);
            }
        }
    }

    return rv;
}

MeshData osc::GenUntexturedSimbodyCylinder(size_t nsides) {
    MeshData rv;
    rv.reserve(4*3*nsides);

    constexpr float topY = +1.0f;
    constexpr float bottomY = -1.0f;
    const float stepAngle = 2.0f*fpi / nsides;

    unsigned short index = 0;
    auto push = [&rv, &index](glm::vec3 const& pos, glm::vec3 const& norm) {
        rv.verts.push_back(pos);
        rv.normals.push_back(norm);
        rv.indices.push_back(index++);
    };

    // top
    {
        glm::vec3 normal = {0.0f, 1.0f, 0.0f};
        glm::vec3 middle = {0.0f, topY, 0.0f};

        for (size_t i = 0; i < nsides; ++i) {
            float thetaStart = i * stepAngle;
            float thetaEnd = (i + 1) * stepAngle;

            // note: ensure these wind CCW (culling)
            glm::vec3 e1 = {cosf(thetaEnd), topY, sinf(thetaEnd)};
            glm::vec3 e2 = {cosf(thetaStart), topY, sinf(thetaStart)};

            push(middle, normal);
            push(e1, normal);
            push(e2, normal);
        }
    }

    // bottom
    {
        glm::vec3 normal = {0.0f, -1.0f, 0.0f};
        glm::vec3 middle = {0.0f, bottomY, 0.0f};

        for (size_t i = 0; i < nsides; ++i) {
            float thetaStart = i * stepAngle;
            float thetaEnd = (i + 1) * stepAngle;

            // note: ensure these wind CCW (culling)
            glm::vec3 e1 = {cosf(thetaStart), bottomY, sinf(thetaStart)};
            glm::vec3 e2 = {cosf(thetaEnd), bottomY, sinf(thetaEnd)};

            push(middle, normal);
            push(e1, normal);
            push(e2, normal);
        }
    }

    // sides
    for (size_t i = 0; i < nsides; ++i) {
        float thetaStart = i * stepAngle;
        float thetaEnd = thetaStart + stepAngle;
        float normNormtheta = thetaStart + (stepAngle / 2.0f);

        glm::vec3 normal = {cosf(normNormtheta), 0.0f, sinf(normNormtheta)};
        glm::vec3 top1 = {cosf(thetaStart), topY, sinf(thetaStart)};
        glm::vec3 top2 = {cosf(thetaEnd), topY, sinf(thetaEnd)};
        glm::vec3 bottom1 = {top1.x, bottomY, top1.z};
        glm::vec3 bottom2 = {top2.x, bottomY, top2.z};

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

MeshData osc::GenUntexturedSimbodyCone(size_t nsides) {
    MeshData rv;
    rv.reserve(2*3*nsides);

    constexpr float topY = +1.0f;
    constexpr float bottomY = -1.0f;
    const float stepAngle = 2.0f*fpi / nsides;

    unsigned short index = 0;
    auto push = [&rv, &index](glm::vec3 const& pos, glm::vec3 const& norm) {
        rv.verts.push_back(pos);
        rv.normals.push_back(norm);
        rv.indices.push_back(index++);
    };

    // bottom
    {
        glm::vec3 normal = {0.0f, -1.0f, 0.0f};
        glm::vec3 middle = {0.0f, bottomY, 0.0f};

        for (size_t i = 0; i < nsides; ++i) {
            float thetaStart = i * stepAngle;
            float thetaEnd = (i + 1) * stepAngle;

            glm::vec3 p1 = {cosf(thetaStart), bottomY, sinf(thetaStart)};
            glm::vec3 p2 = {cosf(thetaEnd), bottomY, sinf(thetaEnd)};

            push(middle, normal);
            push(p1, normal);
            push(p2, normal);
        }
    }

    // sides
    {
        for (size_t i = 0; i < nsides; ++i) {
            float thetaStart = i * stepAngle;
            float thetaEnd = (i + 1) * stepAngle;

            glm::vec3 points[3] = {
                {0.0f, topY, 0.0f},
                {cosf(thetaEnd), bottomY, sinf(thetaEnd)},
                {cosf(thetaStart), bottomY, sinf(thetaStart)},
            };

            glm::vec3 normal = TriangleNormal(points);

            push(points[0], normal);
            push(points[1], normal);
            push(points[2], normal);
        }
    }

    return rv;
}

MeshData osc::GenNbyNGrid(size_t n) {
    static constexpr float z = 0.0f;
    static constexpr float min = -1.0f;
    static constexpr float max = 1.0f;

    float stepSize = (max - min) / static_cast<float>(n);

    size_t nlines = n + 1;

    MeshData rv;
    rv.verts.reserve(4*nlines);
    rv.indices.reserve(4*nlines);
    rv.topography = MeshTopography::Lines;

    unsigned short index = 0;
    auto push = [&index, &rv](glm::vec3 const& pos) {
        rv.verts.push_back(pos);
        rv.indices.push_back(index++);
    };

    // lines parallel to X axis
    for (size_t i = 0; i < nlines; ++i) {
        float y = min + i * stepSize;

        push({-1.0f, y, z});
        push({+1.0f, y, z});
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < nlines; ++i) {
        float x = min + i * stepSize;

        push({x, -1.0f, z});
        push({x, +1.0f, z});
    }

    return rv;
}

MeshData osc::GenYLine() {
    MeshData rv;
    rv.verts = {{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    rv.indices = {0, 1};
    rv.topography = MeshTopography::Lines;
    return rv;
}

MeshData osc::GenCube() {
    MeshData rv;
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

MeshData osc::GenCubeLines() {
    MeshData rv;
    rv.verts.reserve(g_CubeEdgeLines.size());
    rv.indices.reserve(g_CubeEdgeLines.size());
    rv.topography = MeshTopography::Lines;

    unsigned short index = 0;
    for (auto const& v : g_CubeEdgeLines) {
        rv.verts.push_back(v.pos);
        rv.indices.push_back(index++);
    }

    return rv;
}

MeshData osc::GenCircle(size_t nsides) {
    MeshData rv;
    rv.verts.reserve(3*nsides);
    rv.topography = MeshTopography::Lines;

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

glm::vec2 osc::TopleftRelPosToNDCPoint(glm::vec2 relpos) {
    relpos.y = 1.0f - relpos.y;
    return 2.0f*relpos - 1.0f;
}

// converts a topleft-origin RELATIVE `pos` (0 to 1 in XY, starting topleft) into
// the equivalent POINT on the front of the NDC cube (i.e. "as if" a viewer was there)
//
// i.e. {X_ndc, Y_ndc, -1.0f, 1.0f}
glm::vec4 osc::TopleftRelPosToNDCCube(glm::vec2 relpos) {
    return {TopleftRelPosToNDCPoint(relpos), -1.0f, 1.0f};
}

osc::PolarPerspectiveCamera::PolarPerspectiveCamera() :
    radius{1.0f},
    theta{0.0f},
    phi{0.0f},
    focusPoint{0.0f, 0.0f, 0.0f},
    fov{120.0f},
    znear{0.1f},
    zfar{100.0f}
{
}

void osc::PolarPerspectiveCamera::reset()
{
    *this = {};
}

void osc::PolarPerspectiveCamera::pan(float aspectRatio, glm::vec2 delta) noexcept {
    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    float xAmt = delta.x * aspectRatio * (2.0f * tanf(fov / 2.0f) * radius);
    float yAmt = -delta.y * (1.0f / aspectRatio) * (2.0f * tanf(fov / 2.0f) * radius);

    // this assumes the scene is not rotated, so we need to rotate these
    // axes to match the scene's rotation
    glm::vec4 defaultPanningAx = {xAmt, yAmt, 0.0f, 1.0f};
    auto rotTheta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto thetaVec = glm::normalize(glm::vec3{sinf(theta), 0.0f, cosf(theta)});
    auto phiAxis = glm::cross(thetaVec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rotPhi = glm::rotate(glm::identity<glm::mat4>(), phi, phiAxis);

    glm::vec4 panningAxes = rotPhi * rotTheta * defaultPanningAx;
    focusPoint.x += panningAxes.x;
    focusPoint.y += panningAxes.y;
    focusPoint.z += panningAxes.z;
}

void osc::PolarPerspectiveCamera::drag(glm::vec2 delta) noexcept {
    theta += 2.0f * fpi * -delta.x;
    phi += 2.0f * fpi * delta.y;
}

void osc::PolarPerspectiveCamera::rescaleZNearAndZFarBasedOnRadius() noexcept {
    // znear and zfar are only really dicated by the camera's radius, because
    // the radius is effectively the distance from the camera's focal point

    znear = 0.02f * radius;
    zfar = 20.0f * radius;
}

glm::mat4 osc::PolarPerspectiveCamera::getViewMtx() const noexcept {
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    // this maths is a complete shitshow and I apologize. It just happens to work for now. It's a polar coordinate
    // system that shifts the world based on the camera pan

    auto rotTheta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto thetaVec = glm::normalize(glm::vec3{sinf(theta), 0.0f, cosf(theta)});
    auto phiAxis = glm::cross(thetaVec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rotPhi = glm::rotate(glm::identity<glm::mat4>(), -phi, phiAxis);
    auto panTranslate = glm::translate(glm::identity<glm::mat4>(), focusPoint);
    return glm::lookAt(glm::vec3(0.0f, 0.0f, radius), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3{0.0f, 1.0f, 0.0f}) *
           rotTheta * rotPhi * panTranslate;
}

glm::mat4 osc::PolarPerspectiveCamera::getProjMtx(float aspectRatio) const noexcept {
    return glm::perspective(fov, aspectRatio, znear, zfar);
}

glm::vec3 osc::PolarPerspectiveCamera::getPos() const noexcept {
    float x = radius * sinf(theta) * cosf(phi);
    float y = radius * sinf(phi);
    float z = radius * cosf(theta) * cosf(phi);

    return -focusPoint + glm::vec3{x, y, z};
}

glm::vec2 osc::PolarPerspectiveCamera::projectOntoScreenRect(glm::vec3 const& worldspaceLoc, Rect const& screenRect) const noexcept {

    glm::vec2 dims = RectDims(screenRect);
    glm::mat4 MV = getProjMtx(dims.x/dims.y) * getViewMtx();

    glm::vec4 ndc = MV * glm::vec4{worldspaceLoc, 1.0f};
    ndc /= ndc.w;  // perspective divide

    glm::vec2 ndc2D;
    ndc2D = {ndc.x, -ndc.y};        // [-1, 1], Y points down
    ndc2D += 1.0f;                  // [0, 2]
    ndc2D *= 0.5f;                  // [0, 1]
    ndc2D *= dims;                  // [0, w]
    ndc2D += screenRect.p1;         // [x, x + w]

    return ndc2D;
}

Line osc::PolarPerspectiveCamera::unprojectTopLeftPosToWorldRay(glm::vec2 pos, glm::vec2 dims) const noexcept {
    glm::mat4 projMtx = getProjMtx(dims.x/dims.y);
    glm::mat4 viewMtx = getViewMtx();

    // position of point, as if it were on the front of the 3D NDC cube
    glm::vec4 lineOriginNDC = TopleftRelPosToNDCCube(pos/dims);

    glm::vec4 lineOriginView = glm::inverse(projMtx) * lineOriginNDC;
    lineOriginView /= lineOriginView.w;  // perspective divide

    // location of mouse in worldspace
    glm::vec3 lineOriginWorld = glm::vec3{glm::inverse(viewMtx) * lineOriginView};

    // direction vector from camera to mouse location (i.e. the projection)
    glm::vec3 lineDirWorld = glm::normalize(lineOriginWorld - this->getPos());

    Line rv;
    rv.dir = lineDirWorld;
    rv.origin = lineOriginWorld;
    return rv;
}

osc::EulerPerspectiveCamera::EulerPerspectiveCamera() :
    pos{0.0f, 0.0f, 0.0f},
    pitch{0.0f},
    yaw{-fpi/2.0f},
    fov{fpi * 70.0f/180.0f},
    znear{0.1f},
    zfar{1000.0f} {
}

glm::vec3 osc::EulerPerspectiveCamera::getFront() const noexcept {
    return glm::normalize(glm::vec3{
        cosf(yaw) * cosf(pitch),
        sinf(pitch),
        sinf(yaw) * cosf(pitch),
    });
}

glm::vec3 osc::EulerPerspectiveCamera::getUp() const noexcept {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 osc::EulerPerspectiveCamera::getRight() const noexcept {
    return glm::normalize(glm::cross(getFront(), getUp()));
}

glm::mat4 osc::EulerPerspectiveCamera::getViewMtx() const noexcept {
    return glm::lookAt(pos, pos + getFront(), getUp());
}

glm::mat4 osc::EulerPerspectiveCamera::getProjMtx(float aspect_ratio) const noexcept {
    return glm::perspective(fov, aspect_ratio, znear, zfar);
}

#include "Geometry.hpp"

#include "src/Maths/Disc.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/Plane.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Segment.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Maths/Sphere.hpp"
#include "src/Utils/Assertions.hpp"

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    struct QuadraticFormulaResult final {
        bool computeable;
        float x0;
        float x1;
    };
}

// solve a quadratic formula
//
// only real-valued results supported - no complex-plane results
static QuadraticFormulaResult solveQuadratic(float a, float b, float c) {
    QuadraticFormulaResult res;

    // b2 - 4ac
    float discriminant = b*b - 4.0f*a*c;

    if (discriminant < 0.0f)
    {
        res.computeable = false;
        return res;
    }

    // q = -1/2 * (b +- sqrt(b2 - 4ac))
    float q = -0.5f * (b + std::copysign(std::sqrt(discriminant), b));

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


static osc::RayCollision GetRayCollisionSphere(osc::Sphere const& s, osc::Line const& l) noexcept
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    osc::RayCollision rv;

    glm::vec3 L = s.origin - l.origin;  // line origin to sphere origin
    float tca = glm::dot(L, l.dir);  // projected line from middle of hitline to sphere origin

    if (tca < 0.0f)
    {
        // line is pointing away from the sphere
        rv.hit = false;
        return rv;
    }

    float d2 = glm::dot(L, L) - tca*tca;
    float r2 = s.radius * s.radius;

    if (d2 > r2)
    {
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

static osc::RayCollision GetRayCollisionSphereAnalytic(osc::Sphere const& s, osc::Line const& l) noexcept
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    osc::RayCollision rv;

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

    if (!ok)
    {
        rv.hit = false;
        return rv;
    }

    // ensure X0 < X1
    if (t0 > t1)
    {
        std::swap(t0, t1);
    }

    // ensure it's in front
    if (t0 < 0.0f)
    {
        t0 = t1;
        if (t0 < 0.0f)
        {
            rv.hit = false;
            return rv;
        }
    }

    rv.hit = true;
    rv.distance = t0;  // other = t1
    return rv;
}


// public API

bool osc::AreAtSameLocation(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    float eps = std::numeric_limits<float>::epsilon();
    float eps2 = eps * eps;
    glm::vec3 b2a = a - b;
    float len2 = glm::dot(b2a, b2a);
    return len2 > eps2;
}

glm::vec3 osc::Min(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    return glm::vec3
    {
        std::min(a.x, b.x),
        std::min(a.y, b.y),
        std::min(a.z, b.z),
    };
}

glm::vec2 osc::Min(glm::vec2 const& a, glm::vec2 const& b) noexcept
{
    return glm::vec2
    {
        std::min(a.x, b.x),
        std::min(a.y, b.y),
    };
}

glm::vec3 osc::Max(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    return glm::vec3
    {
        std::max(a.x, b.x),
        std::max(a.y, b.y),
        std::max(a.z, b.z),
    };
}

glm::vec2 osc::Max(glm::vec2 const& a, glm::vec2 const& b) noexcept
{
    return glm::vec2
    {
        std::max(a.x, b.x),
        std::max(a.y, b.y),
    };
}

glm::vec3::length_type osc::LongestDimIndex(glm::vec3 const& v) noexcept
{
    if (v.x > v.y && v.x > v.z)
    {
        return 0;  // X is longest
    }
    else if (v.y > v.z)
    {
        return 1;  // Y is longest
    }
    else
    {
        return 2;  // Z is longest
    }
}

glm::vec2::length_type osc::LongestDimIndex(glm::vec2 v) noexcept
{
    if (v.x > v.y)
    {
        return 0;  // X is longest
    }
    else
    {
        return 1;
    }
}

glm::ivec2::length_type osc::LongestDimIndex(glm::ivec2 v) noexcept
{
    if (v.x > v.y)
    {
        return 0;  // X is longest
    }
    else
    {
        return 1;
    }
}

float osc::LongestDim(glm::vec3 const& v) noexcept
{
    return v[LongestDimIndex(v)];
}

float osc::LongestDim(glm::vec2 v) noexcept
{
    return v[LongestDimIndex(v)];
}

int osc::LongestDim(glm::ivec2 v) noexcept
{
    return v[LongestDimIndex(v)];
}

float osc::AspectRatio(glm::ivec2 v) noexcept
{
    return static_cast<float>(v.x) / static_cast<float>(v.y);
}

float osc::AspectRatio(glm::vec2 v) noexcept
{
    return v.x/v.y;
}

glm::vec3 osc::Midpoint(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    return (a+b)/2.0f;
}

glm::vec3 osc::KahanSum(glm::vec3 const* vs, size_t n) noexcept
{
    glm::vec3 sum{};  // accumulator
    glm::vec3 c{};    // running compensation of low-order bits

    for (size_t i = 0; i < n; ++i)
    {
        glm::vec3 y = vs[i] - c;  // subtract the compensation amount from the next number
        glm::vec3 t = sum + y;    // perform the summation (might lose information)
        c = (t - sum) - y;        // (t-sum) yields the retained (high-order) parts of `y`, so `c` contains the "lost" information
        sum = t;                  // CAREFUL: algebreically, `c` always == 0 - despite the computer's (actual) limited precision, the compiler might elilde all of this
    }

    return sum;
}

glm::vec3 osc::NumericallyStableAverage(glm::vec3 const* vs, size_t n) noexcept
{
    glm::vec3 sum = KahanSum(vs, n);
    return sum / static_cast<float>(n);
}

glm::vec3 osc::TriangleNormal(glm::vec3 const* v) noexcept
{
    glm::vec3 ab = v[1] - v[0];
    glm::vec3 ac = v[2] - v[0];
    glm::vec3 perpendiular = glm::cross(ab, ac);
    return glm::normalize(perpendiular);
}

glm::vec3 osc::TriangleNormal(glm::vec3 const& a, glm::vec3 const& b, glm::vec3 const& c) noexcept
{
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 perpendiular = glm::cross(ab, ac);
    return glm::normalize(perpendiular);
}

glm::mat3 osc::ToNormalMatrix(glm::mat4 const& m) noexcept
{
    glm::mat3 topLeft{m};
    return glm::inverse(glm::transpose(topLeft));
}

glm::mat3 osc::ToNormalMatrix(glm::mat4x3 const& m) noexcept
{
    glm::mat3 topLeft{m};
    return glm::inverse(glm::transpose(topLeft));
}

glm::mat4 osc::Dir1ToDir2Xform(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    float cosAng = glm::dot(a, b);

    if (std::fabs(cosAng) > 0.999f)
    {
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

glm::vec3 osc::ExtractEulerAngleXYZ(glm::mat4 const& m) noexcept
{
    glm::vec3 v;
    glm::extractEulerAngleXYZ(m, v.x, v.y, v.z);
    return v;
}

float osc::Area(Rect const& r) noexcept
{
    auto d = Dimensions(r);
    return d.x * d.y;
}

glm::vec2 osc::Dimensions(Rect const& r) noexcept
{
    return glm::abs(r.p2 - r.p1);
}

glm::vec2 osc::BottomLeft(Rect const& r) noexcept
{
    return glm::vec2{glm::min(r.p1.x, r.p2.x), glm::max(r.p1.y, r.p2.y)};
}

float osc::AspectRatio(Rect const& r) noexcept
{
    glm::vec2 dims = Dimensions(r);
    return dims.x/dims.y;
}


bool osc::IsPointInRect(Rect const& r, glm::vec2 const& p) noexcept
{
    glm::vec2 relPos = p - r.p1;
    glm::vec2 dims = Dimensions(r);
    return (0.0f <= relPos.x && relPos.x <= dims.x) && (0.0f <= relPos.y && relPos.y <= dims.y);
}

osc::Sphere osc::BoundingSphereOf(glm::vec3 const* vs, size_t n) noexcept
{
    AABB aabb = AABBFromVerts(vs, n);

    Sphere rv{};
    rv.origin = (aabb.min + aabb.max) / 2.0f;
    rv.radius = 0.0f;

    // edge-case: no points provided
    if (n == 0)
    {
        return rv;
    }

    float biggestR2 = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        glm::vec3 const& pos = vs[i];
        glm::vec3 pos2rv = pos - rv.origin;
        float r2 = glm::dot(pos2rv, pos2rv);
        biggestR2 = std::max(biggestR2, r2);
    }

    rv.radius = glm::sqrt(biggestR2);

    return rv;
}

glm::mat4 osc::FromUnitSphereMat4(Sphere const& s) noexcept
{
    return glm::translate(glm::mat4{1.0f}, s.origin) * glm::scale(glm::mat4{1.0f}, {s.radius, s.radius, s.radius});
}

glm::mat4 osc::SphereToSphereMat4(Sphere const& a, Sphere const& b) noexcept
{
    float scale = b.radius/a.radius;
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, glm::vec3{scale, scale, scale});
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, b.origin - a.origin);
    return mover * scaler;
}

osc::AABB osc::ToAABB(Sphere const& s) noexcept
{
    AABB rv;
    rv.min = s.origin - s.radius;
    rv.max = s.origin + s.radius;
    return rv;
}

osc::Line osc::TransformLine(Line const& l, glm::mat4 const& m) noexcept
{
    Line rv;
    rv.dir = m * glm::vec4{l.dir, 0.0f};
    rv.origin = m * glm::vec4{l.origin, 1.0f};
    return rv;
}

glm::mat4 osc::DiscToDiscMat4(Disc const& a, Disc const& b) noexcept
{
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
    if (cosTheta > 0.9999f)
    {
        rotator = glm::mat4{1.0f};
    }
    else
    {
        float theta = glm::acos(cosTheta);
        glm::vec3 axis = glm::cross(a.normal, b.normal);
        rotator = glm::rotate(glm::mat4{1.0f}, theta, axis);
    }

    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, b.origin-a.origin);

    return translator * rotator * scaler;
}

glm::vec3 osc::Midpoint(AABB const& a) noexcept
{
    return (a.min + a.max)/2.0f;
}

glm::vec3 osc::Dimensions(AABB const& a) noexcept
{
    return a.max - a.min;
}

osc::AABB osc::Union(AABB const& a, AABB const& b) noexcept
{
    return AABB
    {
        Min(a.min, b.min),
        Max(a.max, b.max)
    };
}

osc::AABB osc::Union(void const* data, size_t n, size_t stride, size_t offset)
{
    if (n == 0)
    {
        return AABB{};
    }

    OSC_ASSERT(reinterpret_cast<uintptr_t>(data) % alignof(AABB) == 0 && "possible unaligned load detected: this will cause bugs on systems that only support aligned loads (e.g. ARM)");
    OSC_ASSERT(static_cast<uintptr_t>(offset) % alignof(AABB) == 0 && "possible unaligned load detected: this will cause bugs on systems that only support aligned loads (e.g. ARM)");

    unsigned char const* firstPtr = reinterpret_cast<unsigned char const*>(data);

    AABB rv = *reinterpret_cast<AABB const*>(firstPtr + offset);
    for (size_t i = 1; i < n; ++i)
    {
        unsigned char const* elPtr = firstPtr + (i*stride) + offset;
        AABB const& aabb = *reinterpret_cast<AABB const*>(elPtr);
        rv = Union(rv, aabb);
    }

    return rv;
}

bool osc::IsEffectivelyEmpty(AABB const& a) noexcept
{
    for (int i = 0; i < 3; ++i)
    {
        if (a.min[i] == a.max[i])
        {
            return true;
        }
    }
    return false;
}

glm::vec3::length_type osc::LongestDimIndex(AABB const& a) noexcept
{
    return LongestDimIndex(Dimensions(a));
}

float osc::LongestDim(AABB const& a) noexcept
{
    glm::vec3 dims = Dimensions(a);
    float rv = dims[0];
    rv = std::max(rv, dims[1]);
    rv = std::max(rv, dims[2]);
    return rv;
}

std::array<glm::vec3, 8> osc::ToCubeVerts(AABB const& aabb) noexcept
{
    glm::vec3 d = Dimensions(aabb);

    std::array<glm::vec3, 8> rv;
    rv[0] = aabb.min;
    rv[1] = aabb.max;
    int pos = 2;
    for (int i = 0; i < 3; ++i)
    {
        glm::vec3 min = aabb.min;
        min[i] += d[i];
        glm::vec3 max = aabb.max;
        max[i] -= d[i];
        rv[pos++] = min;
        rv[pos++] = max;
    }
    return rv;
}

osc::AABB osc::TransformAABB(AABB const& aabb, glm::mat4 const& m) noexcept
{
    auto verts = ToCubeVerts(aabb);

    for (auto& vert : verts)
    {
        glm::vec4 p = m * glm::vec4{vert, 1.0f};
        vert = glm::vec3{p / p.w}; // perspective divide
    }

    return AABBFromVerts(verts.data(), verts.size());
}

osc::AABB osc::TransformAABB(AABB const& aabb, Transform const& t) noexcept
{
    auto verts = ToCubeVerts(aabb);

    for (auto& vert : verts)
    {
        vert = t * vert;
    }

    return AABBFromVerts(verts.data(), verts.size());
}

osc::AABB osc::AABBFromVerts(glm::vec3 const* vs, size_t n) noexcept
{
    AABB rv{};

    // edge-case: no points provided
    if (n == 0)
    {
        return rv;
    }

    rv.min =
    {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };

    rv.max =
    {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    // otherwise, compute bounds
    for (size_t i = 0; i < n; ++i)
    {
        glm::vec3 const& pos = vs[i];
        rv.min = Min(rv.min, pos);
        rv.max = Max(rv.max, pos);
    }

    return rv;
}

osc::AABB osc::AABBFromIndexedVerts(nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices)
{
    AABB rv{};

    // edge-case: no points provided
    if (indices.empty())
    {
        return rv;
    }

    rv.min =
    {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };

    rv.max =
    {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    for (uint32_t idx : indices)
    {
        glm::vec3 const& pos = verts[idx];
        rv.min = Min(rv.min, pos);
        rv.max = Max(rv.max, pos);
    }

    return rv;
}

osc::AABB osc::AABBFromIndexedVerts(nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices)
{
    AABB rv{};

    // edge-case: no points provided
    if (indices.empty())
    {
        return rv;
    }

    rv.min =
    {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };

    rv.max =
    {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    for (uint16_t idx : indices)
    {
        glm::vec3 const& pos = verts[idx];
        rv.min = Min(rv.min, pos);
        rv.max = Max(rv.max, pos);
    }

    return rv;
}

glm::mat4 osc::SegmentToSegmentMat4(Segment const& a, Segment const& b) noexcept
{
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

osc::Transform osc::SegmentToSegmentTransform(Segment const& a, Segment const& b) noexcept
{
    glm::vec3 aLine = a.p2 - a.p1;
    glm::vec3 bLine = b.p2 - b.p1;

    float aLen = glm::length(aLine);
    float bLen = glm::length(bLine);

    glm::vec3 aDir = aLine/aLen;
    glm::vec3 bDir = bLine/bLen;

    glm::vec3 aMid = (a.p1 + a.p2)/2.0f;
    glm::vec3 bMid = (b.p1 + b.p2)/2.0f;

    // for scale: LERP [0,1] onto [1,l] along original direction
    Transform t;
    t.rotation = glm::rotation(aDir, bDir);
    t.scale = glm::vec3{1.0f, 1.0f, 1.0f} + ((bLen/aLen - 1.0f)*aDir);
    t.position = bMid - aMid;

    return t;
}

osc::RayCollision osc::GetRayCollisionSphere(Line const& l, Sphere const& s) noexcept
{
    return GetRayCollisionSphereAnalytic(s, l);
}

osc::RayCollision osc::GetRayCollisionAABB(Line const& l, AABB const& bb) noexcept
{
    RayCollision rv;

    float t0 = std::numeric_limits<float>::lowest();
    float t1 = std::numeric_limits<float>::max();

    // intersect the ray with each axis-aligned slab for each dimension
    //
    // i.e. figure out where the line intersects the front+back of the AABB
    //      in (e.g.) X, then Y, then Z, and intersect those interactions such
    //      that if the intersection is ever empty (or, negative here) then there
    //      is no intersection
    for (int i = 0; i < 3; ++i)
    {
        float invDir = 1.0f / l.dir[i];
        float tNear = (bb.min[i] - l.origin[i]) * invDir;
        float tFar = (bb.max[i] - l.origin[i]) * invDir;
        if (tNear > tFar)
        {
            std::swap(tNear, tFar);
        }
        t0 = std::max(t0, tNear);
        t1 = std::min(t1, tFar);

        if (t0 > t1)
        {
            rv.hit = false;
            return rv;
        }
    }

    rv.hit = true;
    rv.distance = t0;  // other == t1
    return rv;
}

osc::RayCollision osc::GetRayCollisionPlane(Line const& l, Plane const& p) noexcept
{
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

    if (std::abs(denominator) > 1e-6)
    {
        float numerator = glm::dot(p.origin - l.origin, p.normal);
        rv.hit = true;
        rv.distance = numerator / denominator;
        return rv;
    }
    else
    {
        // the line is *very* parallel to the plane, which could cause
        // some divide-by-zero havok: pretend it didn't intersect
        rv.hit = false;
        return rv;
    }
}

osc::RayCollision osc::GetRayCollisionDisc(Line const& l, Disc const& d) noexcept
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection

    // think of this as a ray-plane intersection test with the additional
    // constraint that the ray has to be within the radius of the disc

    RayCollision rv;

    Plane p;
    p.origin = d.origin;
    p.normal = d.normal;

    auto [hitsPlane, t] = GetRayCollisionPlane(l, p);

    if (!hitsPlane)
    {
        rv.hit = false;
        return rv;
    }

    // figure out whether the plane hit is within the disc's radius
    glm::vec3 pos = l.origin + t*l.dir;
    glm::vec3 v = pos - d.origin;
    float d2 = glm::dot(v, v);
    float r2 = glm::dot(d.radius, d.radius);

    if (d2 > r2)
    {
        rv.hit = false;
        return rv;
    }

    rv.hit = true;
    rv.distance = t;
    return rv;
}

osc::RayCollision osc::GetRayCollisionTriangle(Line const& l, glm::vec3 const* v) noexcept
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution

    RayCollision rv;

    // compute triangle normal
    glm::vec3 N = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));

    // compute dot product between normal and ray
    float NdotR = glm::dot(N, l.dir);

    // if the dot product is small, then the ray is probably very parallel to
    // the triangle (or, perpendicular to the normal) and doesn't intersect
    if (std::abs(NdotR) < std::numeric_limits<float>::epsilon())
    {
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
    if (t < 0.0f)
    {
        rv.hit = false;
        return rv;
    }

    // intersection point on triangle plane, computed from line equation
    glm::vec3 P = l.origin + t*l.dir;

    // figure out if that point is inside the triangle's bounds using the
    // "inside-outside" test

    // test each triangle edge: {0, 1}, {1, 2}, {2, 0}
    for (int i = 0; i < 3; ++i)
    {
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
        if (glm::dot(ax, N) < 0.0f)
        {
            rv.hit = false;
            return rv;
        }
    }

    rv.hit = true;
    rv.distance = t;
    return rv;
}

osc::Transform osc::SimbodyCylinderToSegmentTransform(Segment const& s, float radius) noexcept
{
    Segment cylinderLine{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Transform t = SegmentToSegmentTransform(cylinderLine, s);
    t.scale.x = radius;
    t.scale.z = radius;
    return t;
}

osc::Transform osc::SimbodyConeToSegmentTransform(Segment const& s, float radius) noexcept
{
    return SimbodyCylinderToSegmentTransform(s, radius);
}

glm::vec2 osc::TopleftRelPosToNDCPoint(glm::vec2 relpos)
{
    relpos.y = 1.0f - relpos.y;
    return 2.0f*relpos - 1.0f;
}

// converts a topleft-origin RELATIVE `pos` (0 to 1 in XY, starting topleft) into
// the equivalent POINT on the front of the NDC cube (i.e. "as if" a viewer was there)
//
// i.e. {X_ndc, Y_ndc, -1.0f, 1.0f}
glm::vec4 osc::TopleftRelPosToNDCCube(glm::vec2 relpos)
{
    return {TopleftRelPosToNDCPoint(relpos), -1.0f, 1.0f};
}

glm::mat4 osc::ToMat4(Transform const& t) noexcept
{
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, t.scale);
    glm::mat4 rotater = glm::toMat4(t.rotation);
    glm::mat4 translater = glm::translate(glm::mat4{1.0f}, t.position);

    return translater * rotater * scaler;
}

glm::mat4 osc::ToInverseMat4(Transform const& t) noexcept
{
    glm::mat4 translater = glm::translate(glm::mat4{1.0f}, -t.position);
    glm::mat4 rotater = glm::toMat4(glm::conjugate(t.rotation));
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, 1.0f/t.scale);

    return scaler * rotater * translater;
}

glm::mat3x3 osc::ToNormalMatrix(Transform const& t) noexcept
{
    return glm::toMat3(t.rotation);
}

osc::Transform osc::ToTransform(glm::mat4 const& mtx)
{
    Transform rv;
    glm::vec3 skew;
    glm::vec4 perspective;
    if (!glm::decompose(mtx, rv.scale, rv.rotation, rv.position, skew, perspective))
    {
        throw std::runtime_error{"failed to decompose a matrix into scale, rotation, etc."};
    }
    return rv;
}

glm::vec3 osc::TransformDirection(Transform const& t, glm::vec3 const& localDir) noexcept
{
    return t.rotation * localDir;
}

glm::vec3 osc::InverseTransformDirection(Transform const& t, glm::vec3 const& worldDir) noexcept
{
    return glm::conjugate(t.rotation) * worldDir;
}

glm::vec3 osc::TransformPoint(Transform const& t, glm::vec3 const& localPoint) noexcept
{
    glm::vec3 rv{localPoint};
    rv *= t.scale;
    rv = t.rotation * rv;
    rv += t.position;
    return rv;
}

glm::vec3 osc::InverseTransformPoint(Transform const& t, glm::vec3 const& worldPoint) noexcept
{
    glm::vec3 rv = worldPoint;
    rv -= t.position;
    rv = glm::conjugate(t.rotation) * rv;
    rv /= t.scale;
    return rv;
}

void osc::ApplyWorldspaceRotation(Transform& t,
                                  glm::vec3 const& eulerAngles,
                                  glm::vec3 const& rotationCenter) noexcept
{
    glm::quat q{eulerAngles};
    t.position = q*(t.position - rotationCenter) + rotationCenter;
    t.rotation = glm::normalize(q*t.rotation);
}

glm::vec3 osc::ExtractEulerAngleXYZ(Transform const& t) noexcept
{
    glm::vec3 rv;
    glm::extractEulerAngleXYZ(glm::toMat4(t.rotation), rv.x, rv.y, rv.z);
    return rv;
}

glm::vec3 osc::ExtractExtrinsicEulerAnglesXYZ(Transform const& t) noexcept
{
    return glm::eulerAngles(t.rotation);
}

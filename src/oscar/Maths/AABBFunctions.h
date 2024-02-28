#pragma once

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec3.h>

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace osc
{
    // returns the average centroid of an AABB
    Vec3 centroid(AABB const&);

    // returns the dimensions of an AABB
    Vec3 dimensions(AABB const&);

    // returns the half-widths of an AABB (effectively, dimensions(aabb)/2.0)
    Vec3 half_widths(AABB const&);

    // returns the volume of the AABB
    float volume(AABB const&);

    // returns the smallest AABB that spans both of the provided AABBs
    AABB union_of(AABB const&, AABB const&);

    // returns true if the AABB has no extents in any dimension
    bool is_point(AABB const&);

    // returns true if the AABB is an extent in any dimension is zero
    bool has_zero_volume(AABB const&);

    // returns the eight corner points of the cuboid representation of the AABB
    std::array<Vec3, 8> cuboid_vertices(AABB const&);

    // returns an AABB that has been transformed by the given matrix
    AABB transform_aabb(AABB const&, Mat4 const&);

    // returns an AABB that has been transformed by the given transform
    AABB transform_aabb(AABB const&, Transform const&);

    // returns an AABB that tightly bounds the provided Vec3 (i.e. min=max=Vec3)
    AABB aabb_of(Vec3 const&);

    // returns an AABB that tightly bounds the provided triangle
    AABB aabb_of(Triangle const&);

    // returns an AABB that tightly bounds the provided points
    AABB aabb_of(std::span<Vec3 const>);

    // returns an AABB that tightly bounds the points indexed by the provided 32-bit indices
    AABB aabb_of(std::span<Vec3 const> verts, std::span<uint32_t const> indices);

    // returns an AABB that tightly bounds the points indexed by the provided 16-bit indices
    AABB aabb_of(std::span<Vec3 const> verts, std::span<uint16_t const> indices);

    // (tries to) return a Normalized Device Coordinate (NDC) rectangle, clamped to the NDC clipping
    // bounds ((-1,-1) to (1, 1)), where the rectangle loosely bounds the given worldspace AABB after
    // projecting it into NDC
    //
    // this is useful for (e.g.) roughly figuring out what part of a screen is affected by a given AABB
    std::optional<Rect> loosely_project_into_ndc(
        AABB const&,
        Mat4 const& viewMat,
        Mat4 const& projMat,
        float znear,
        float zfar
    );
}

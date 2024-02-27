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
    // returns an AABB that is "inverted", such that it's minimum is the largest
    // possible value and its maximum is the smallest possible value
    //
    // handy as a "seed" when union-ing many AABBs, because it won't
    AABB InvertedAABB();

    // returns the centerpoint of an AABB
    Vec3 Midpoint(AABB const&);

    // returns the dimensions of an AABB
    Vec3 Dimensions(AABB const&);

    // returns the half-widths of an AABB (effectively, Dimensions(aabb)/2.0)
    Vec3 HalfWidths(AABB const&);

    // returns the volume of the AABB
    float Volume(AABB const&);

    // returns the smallest AABB that spans both of the provided AABBs
    AABB Union(AABB const&, AABB const&);

    // returns true if the AABB has no extents in any dimension
    bool IsAPoint(AABB const&);

    // returns true if the AABB is an extent in any dimension is zero
    bool IsZeroVolume(AABB const&);

    // returns the *index* of the longest dimension of an AABB
    Vec3::size_type LongestDimIndex(AABB const&);

    // returns the length of the longest dimension of an AABB
    float LongestDim(AABB const&);

    // returns the eight corner points of the cuboid representation of the AABB
    std::array<Vec3, 8> ToCubeVerts(AABB const&);

    // returns an AABB that has been transformed by the given matrix
    AABB TransformAABB(AABB const&, Mat4 const&);

    // returns an AABB that has been transformed by the given transform
    AABB TransformAABB(AABB const&, Transform const&);

    // returns an AAB that tightly bounds the provided triangle
    AABB AABBFromTriangle(Triangle const&);

    // returns an AABB that tightly bounds the provided points
    AABB AABBFromVerts(std::span<Vec3 const>);

    // returns an AABB that tightly bounds the provided points (alias that matches BoundingSphereOf, etc.)
    inline AABB BoundingAABBOf(std::span<Vec3 const> vs) { return AABBFromVerts(vs); }

    // returns an AABB that tightly bounds the points indexed by the provided 32-bit indices
    AABB AABBFromIndexedVerts(std::span<Vec3 const> verts, std::span<uint32_t const> indices);

    // returns an AABB that tightly bounds the points indexed by the provided 16-bit indices
    AABB AABBFromIndexedVerts(std::span<Vec3 const> verts, std::span<uint16_t const> indices);

    // (tries to) return a Normalized Device Coordinate (NDC) rectangle, clamped to the NDC clipping
    // bounds ((-1,-1) to (1, 1)), where the rectangle loosely bounds the given worldspace AABB after
    // projecting it into NDC
    std::optional<Rect> AABBToScreenNDCRect(
        AABB const&,
        Mat4 const& viewMat,
        Mat4 const& projMat,
        float znear,
        float zfar
    );
}

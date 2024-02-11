#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>

namespace osc
{
    // generates a textured quad with:
    //
    // - positions: Z == 0, X == [-1, 1], and Y == [-1, 1]
    // - texcoords: (0, 0) to (1, 1)
    Mesh GenerateTexturedQuadMesh();

    // generates UV sphere centered at (0,0,0) with radius = 1
    Mesh GenerateUVSphereMesh(size_t sectors, size_t stacks);

    // generates an untextured cylinder, where the bottom/top are -1.0f/+1.0f in Y
    Mesh GenerateUntexturedYToYCylinderMesh(size_t nsides);

    // generates an untextured cone, where the bottom/top are -1.0f/+1.0f in Y
    Mesh GenerateUntexturedYToYConeMesh(size_t nsides);

    // generates 2D grid lines at Z == 0, X/Y == [-1,+1]
    Mesh GenerateNbyNGridLinesMesh(size_t nticks);

    // generates a single two-point line from (0,-1,0) to (0,+1,0)
    Mesh GenerateYToYLineMesh();

    // generates a cube with [-1,+1] in each dimension
    Mesh GenerateCubeMesh();

    // generates the *lines* of a cube with [-1,+1] in each dimension
    Mesh GenerateCubeLinesMesh();

    // generates a circle at Z == 0, X/Y == [-1, +1] (r = 1)
    Mesh GenerateCircleMesh(size_t nsides);

    // generates a torus with the given number of slices/stacks of triangulated quads
    //
    // x size: [-(torusCenterToTubeCenterRadius + tubeRadius), +(torusCenterToTubeCenterRadius + tubeRadius)]
    // y size: [-(torusCenterToTubeCenterRadius + tubeRadius), +(torusCenterToTubeCenterRadius + tubeRadius)]
    // z size: [-tubeRadius, +tubeRadius]
    Mesh GenerateTorusMesh(
        size_t slices,
        size_t stacks,
        float torusCenterToTubeCenterRadius,
        float tubeRadius
    );

    // generates a steps.x * steps.y (NxM) 2D grid of independent points connected
    // to their nearest neighbour by lines (MeshTopology::Lines), where the
    // lowest X/Y values are min.x/min.y and the highest X/Y values are max.x/max.y
    //
    // i.e. the "lowest" grid point is `min`, the next one is `min + (max-min)/steps`
    Mesh GenerateNxMGridLinesMesh(
        Vec2 min,
        Vec2 max,
        Vec2i steps
    );

    // returns a triangle mesh where each triangle is part of a quad, and each quad is
    // part of steps.x (N) by steps.y (M) grid
    //
    // - the grid spans from (-1.0f, -1.0f) to (+1.0f, +1.0f) (i.e. NDC) with Z = 0.0f
    // - texture coordinates are assigned to each vertex, and span from (0.0f, 0.0f) to (1.0f, 1.0f)
    // - the utility of this is that the grid can be warped to (effectively) warp the texture in 2D space
    Mesh GenerateNxMTriangleQuadGridMesh(Vec2i steps);

    // generates a torus knot, the particular shape of which is defined by a pair of coprime integers
    // `p` and `q`. If `p` and `q` are not coprime, the result will be a torus link
    //
    // inspired by three.js's `TorusKnotGeometry`
    Mesh GenerateTorusKnotMesh(
        float torusRadius = 1.0f,
        float tubeRadius = 0.4f,
        size_t numTubularSegments = 64,
        size_t numRadialSegments = 8,
        size_t p = 2,
        size_t q = 3
    );

    // generates a rectangular cuboid with the given dimensions centered on the origin, with each
    // edge parallel to each axis
    //
    // `segments` affects how many 2-triangle quads may be generated along each dimension
    Mesh GenerateBoxMesh(
        float width = 1.0f,
        float height = 1.0f,
        float depth = 1.0f,
        size_t widthSegments = 1,
        size_t heightSegments = 1,
        size_t depthSegments = 1
    );
}

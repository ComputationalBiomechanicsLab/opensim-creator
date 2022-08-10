#pragma once

#include "src/Graphics/Renderer.hpp"

#include <cstddef>

namespace osc
{
    // generates a textured quad with:
    //
    // - positions: Z == 0, X == [-1, 1], and Y == [-1, 1]
    // - texcoords: (0, 0) to (1, 1)
    experimental::Mesh GenTexturedQuad();

    // generates UV sphere centered at (0,0,0) with radius = 1
    experimental::Mesh GenUntexturedUVSphere(size_t sectors, size_t stacks);

    // generates a "Simbody" cylinder, where the bottom/top are -1.0f/+1.0f in Y
    experimental::Mesh GenUntexturedSimbodyCylinder(size_t nsides);

    // generates a "Simbody" cone, where the bottom/top are -1.0f/+1.0f in Y
    experimental::Mesh GenUntexturedSimbodyCone(size_t nsides);

    // generates 2D grid lines at Z == 0, X/Y == [-1,+1]
    experimental::Mesh GenNbyNGrid(size_t nticks);

    // generates a single two-point line from (0,-1,0) to (0,+1,0)
    experimental::Mesh GenYLine();

    // generates a cube with [-1,+1] in each dimension
    experimental::Mesh GenCube();

    // generates the *lines* of a cube with [-1,+1] in each dimension
    experimental::Mesh GenCubeLines();

    // generates a circle at Z == 0, X/Y == [-1, +1] (r = 1)
    experimental::Mesh GenCircle(size_t nsides);

    // generates a cube that matches some of the LearnOpenGL's cube
    experimental::Mesh GenLearnOpenGLCube();
}

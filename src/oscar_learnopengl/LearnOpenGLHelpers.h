#pragma once

#include <oscar/Graphics/Mesh.h>

namespace osc
{
    // generates a cube that matches the one that's commonly used in
    // LearnOpenGL's tutorial content (i.e. centered on origin with
    // half-extents of 0.5 in each dimension)
    Mesh GenerateLearnOpenGLCubeMesh();
}

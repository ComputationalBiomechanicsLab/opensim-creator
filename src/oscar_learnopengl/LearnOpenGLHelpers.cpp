#include "LearnOpenGLHelpers.h"

#include <oscar/Graphics/MeshGenerators.h>
#include <oscar/Maths/Vec3.h>

using namespace osc;

Mesh osc::GenerateLearnOpenGLCubeMesh()
{
    Mesh cube = GenerateCubeMesh();
    cube.transformVerts({ .scale = Vec3{0.5f} });
    return cube;
}

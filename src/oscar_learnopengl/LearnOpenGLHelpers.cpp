#include "LearnOpenGLHelpers.hpp"

#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Vec3.hpp>

using namespace osc;

Mesh osc::GenerateLearnOpenGLCubeMesh()
{
    Mesh cube = GenerateCubeMesh();
    cube.transformVerts({ .scale = Vec3{0.5f} });
    return cube;
}

#include "LearnOpenGLHelpers.hpp"

#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Vec3.hpp>

using osc::Mesh;

Mesh osc::GenerateLearnOpenGLCubeMesh()
{
    Mesh cube = GenerateCubeMesh();
    cube.transformVerts([](Vec3 v) { return 0.5f * v; });
    return cube;
}

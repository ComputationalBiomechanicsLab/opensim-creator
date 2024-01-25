#include "LearnOpenGLHelpers.hpp"

#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Vec3.hpp>

osc::Mesh osc::GenerateLearnOpenGLCubeMesh()
{
    Mesh cube = GenerateCubeMesh();
    cube.transformVerts([](Vec3& v) { v *= 0.5f; });
    return cube;
}

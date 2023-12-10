#include "LearnOpenGLHelpers.hpp"

#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <span>

osc::Mesh osc::GenLearnOpenGLCube()
{
    Mesh cube = osc::GenCube();
    cube.transformVerts([](Vec3& v) { v *= 0.5f; });
    return cube;
}

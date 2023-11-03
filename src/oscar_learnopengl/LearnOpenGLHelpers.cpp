#include "LearnOpenGLHelpers.hpp"

#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <span>

osc::Mesh osc::GenLearnOpenGLCube()
{
    Mesh cube = osc::GenCube();

    cube.transformVerts([](std::span<Vec3> vs)
    {
        for (Vec3& v : vs)
        {
            v *= 0.5f;
        }
    });

    return cube;
}

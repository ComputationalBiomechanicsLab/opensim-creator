#include "LearnOpenGLHelpers.hpp"

#include <nonstd/span.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Vec3.hpp>

osc::Mesh osc::GenLearnOpenGLCube()
{
    Mesh cube = osc::GenCube();

    cube.transformVerts([](nonstd::span<Vec3> vs)
    {
        for (Vec3& v : vs)
        {
            v *= 0.5f;
        }
    });

    return cube;
}

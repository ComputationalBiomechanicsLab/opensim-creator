#include "LearnOpenGLHelpers.hpp"

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>

osc::Mesh osc::GenLearnOpenGLCube()
{
    osc::Mesh cube = osc::GenCube();

    cube.transformVerts([](nonstd::span<glm::vec3> vs)
    {
        for (glm::vec3& v : vs)
        {
            v *= 0.5f;
        }
    });

    return cube;
}

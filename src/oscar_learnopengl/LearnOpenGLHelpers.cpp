#include "LearnOpenGLHelpers.h"

#include <oscar/Graphics/MeshGenerators.h>
#include <oscar/Maths/Vec3.h>

using namespace osc;

Mesh osc::GenerateLearnOpenGLCubeMesh()
{
    return GenerateBoxMesh();
}

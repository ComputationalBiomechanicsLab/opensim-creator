#include "TPSDocument.hpp"

#include <oscar/Graphics/MeshGenerators.hpp>

osc::TPSDocument::TPSDocument() :
    sourceMesh{GenerateUVSphereMesh(16, 16)},
    destinationMesh{GenerateUntexturedYToYCylinderMesh(16)},
    blendingFactor{1.0f}
{
}

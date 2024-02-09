#include "TPSDocument.h"

#include <oscar/Graphics/MeshGenerators.h>

osc::TPSDocument::TPSDocument() :
    sourceMesh{GenerateUVSphereMesh(16, 16)},
    destinationMesh{GenerateUntexturedYToYCylinderMesh(16)},
    blendingFactor{1.0f}
{
}

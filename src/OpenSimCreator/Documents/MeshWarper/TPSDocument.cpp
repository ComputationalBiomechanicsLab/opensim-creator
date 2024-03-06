#include "TPSDocument.h"

#include <oscar/Graphics/MeshGenerators.h>

osc::TPSDocument::TPSDocument() :
    sourceMesh{GenerateSphereMesh2(1.0f, 16, 16)},
    destinationMesh{GenerateUntexturedYToYCylinderMesh(16)},
    blendingFactor{1.0f}
{
}

#include "TPSDocument.h"

#include <oscar/Graphics/MeshGenerators.h>

osc::TPSDocument::TPSDocument() :
    sourceMesh{GenerateSphereMesh(1.0f, 16, 16)},
    destinationMesh{GenerateCylinderMesh(1.0f, 1.0f, 2.0f, 16)},
    blendingFactor{1.0f}
{
}

#include "TPSDocument.hpp"

#include <oscar/Graphics/MeshGenerators.hpp>

osc::TPSDocument::TPSDocument() :
    sourceMesh{GenSphere(16, 16)},
    destinationMesh{GenUntexturedYToYCylinder(16)},
    blendingFactor{1.0f}
{
}

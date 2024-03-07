#include "TPSDocument.h"

#include <oscar/Graphics/Geometries.h>

osc::TPSDocument::TPSDocument() :
    sourceMesh{SphereGeometry{1.0f, 16, 16}},
    destinationMesh{CylinderGeometry{1.0f, 1.0f, 2.0f, 16}},
    blendingFactor{1.0f}
{}

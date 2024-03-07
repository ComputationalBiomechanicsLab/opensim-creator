#include "TPSDocument.h"

#include <oscar/Graphics/Geometries.h>

osc::TPSDocument::TPSDocument() :
    sourceMesh{SphereGeometry::generate_mesh(1.0f, 16, 16)},
    destinationMesh{CylinderGeometry::generate_mesh(1.0f, 1.0f, 2.0f, 16)},
    blendingFactor{1.0f}
{}

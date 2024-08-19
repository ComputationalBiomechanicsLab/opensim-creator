#include "TPSDocument.h"

#include <oscar/Graphics/Geometries.h>

osc::TPSDocument::TPSDocument() :
    sourceMesh{SphereGeometry{{.num_width_segments = 16, .num_height_segments = 16}}},
    destinationMesh{CylinderGeometry{{.height = 2.0f, .num_radial_segments = 16}}},
    blendingFactor{1.0f},
    recalculateNormals{false}
{}

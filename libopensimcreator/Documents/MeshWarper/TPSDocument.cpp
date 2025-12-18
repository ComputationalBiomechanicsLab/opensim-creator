#include "TPSDocument.h"

#include <liboscar/Graphics/Geometries/CylinderGeometry.h>
#include <liboscar/Graphics/Geometries/SphereGeometry.h>

osc::TPSDocument::TPSDocument() :
    sourceMesh{SphereGeometry{{.num_width_segments = 16, .num_height_segments = 16}}.mesh()},
    destinationMesh{CylinderGeometry{{.height = 2.0f, .num_radial_segments = 16}}.mesh()},
    blendingFactor{1.0f},
    recalculateNormals{false},
    sourceLandmarksPrescale{1.0f},
    destinationLandmarksPrescale{1.0f},

    // note: These should ideally match the model warper's defaults (#1122).
    applyAffineTranslation{false},
    applyAffineScale{true},
    applyAffineRotation{false},
    applyNonAffineWarp{true}
{}

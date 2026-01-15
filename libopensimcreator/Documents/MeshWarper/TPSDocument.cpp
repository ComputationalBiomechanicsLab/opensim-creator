#include "TPSDocument.h"

#include <liboscar/graphics/geometries/cylinder_geometry.h>
#include <liboscar/graphics/geometries/sphere_geometry.h>

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

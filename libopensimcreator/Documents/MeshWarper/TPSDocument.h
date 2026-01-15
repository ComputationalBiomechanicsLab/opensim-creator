#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h>

#include <liboscar/graphics/mesh.h>

#include <vector>

namespace osc
{
    // a TPS document
    //
    // a central datastructure that the user edits in-place via the UI
    struct TPSDocument final {
        TPSDocument();

        Mesh sourceMesh;
        Mesh destinationMesh;
        std::vector<TPSDocumentLandmarkPair> landmarkPairs;
        std::vector<TPSDocumentNonParticipatingLandmark> nonParticipatingLandmarks;
        float blendingFactor;
        bool recalculateNormals;
        float sourceLandmarksPrescale;
        float destinationLandmarksPrescale;
        bool applyAffineTranslation;
        bool applyAffineScale;
        bool applyAffineRotation;
        bool applyNonAffineWarp;
    };
}

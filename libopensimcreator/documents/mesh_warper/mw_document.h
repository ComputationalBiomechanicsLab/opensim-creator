#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_non_participating_landmark.h>

#include <liboscar/graphics/mesh.h>

#include <vector>

namespace osc
{
    /// Represents the document that the mesh warper UI edits in-place.
    struct MwDocument final {
        MwDocument();

        Mesh sourceMesh;
        Mesh destinationMesh;
        std::vector<MwDocumentLandmarkPair> landmarkPairs;
        std::vector<MwDocumentNonParticipatingLandmark> nonParticipatingLandmarks;
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

#pragma once

#include <libOpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <libOpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h>

#include <liboscar/Graphics/Mesh.h>

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
    };
}

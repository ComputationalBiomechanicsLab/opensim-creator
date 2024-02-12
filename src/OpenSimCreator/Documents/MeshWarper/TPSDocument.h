#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h>

#include <oscar/Graphics/Mesh.h>

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
    };
}

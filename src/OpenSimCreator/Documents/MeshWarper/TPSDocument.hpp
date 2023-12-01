#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.hpp>

#include <oscar/Graphics/Mesh.hpp>

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

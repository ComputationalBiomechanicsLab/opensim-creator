#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.hpp>

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <cstddef>
#include <vector>

namespace osc
{
    // a TPS document
    //
    // a central datastructure that the user edits in-place via the UI
    struct TPSDocument final {
        Mesh sourceMesh = GenSphere(16, 16);
        Mesh destinationMesh = GenUntexturedYToYCylinder(16);
        std::vector<TPSDocumentLandmarkPair> landmarkPairs;
        std::vector<TPSDocumentNonParticipatingLandmark> nonParticipatingLandmarks;
        float blendingFactor = 1.0f;
    };
}

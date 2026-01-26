#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentHelpers.h>

#include <libopynsim/utilities/simbody_x_oscar.h>
#include <libopynsim/utilities/tps3d.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector3.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

namespace rgs = std::ranges;

namespace osc
{
    // TPS result cache
    //
    // caches the result of an (expensive) TPS warp of the mesh by checking
    // whether the warping parameters have changed
    class TPSResultCache final {
    public:
        const Mesh& getWarpedMesh(const TPSDocument& doc)
        {
            updateAll(doc);
            return m_CachedResultMesh;
        }

        std::span<const Vector3> getWarpedNonParticipatingLandmarkLocations(const TPSDocument& doc)
        {
            updateAll(doc);
            static_assert(alignof(decltype(m_CachedResultNonParticipatingLandmarks.front())) == alignof(SimTK::fVec3));
            static_assert(sizeof(decltype(m_CachedResultNonParticipatingLandmarks.front())) == sizeof(SimTK::fVec3));
            const auto* ptr = std::launder(reinterpret_cast<const Vector3*>(m_CachedResultNonParticipatingLandmarks.data()));
            return {ptr, m_CachedResultNonParticipatingLandmarks.size()};
        }

    private:
        void updateAll(const TPSDocument& doc)
        {
            const bool updatedCoefficients = updateCoefficients(doc);
            const bool updatedNonParticipatingLandmarks = updateSourceNonParticipatingLandmarks(doc);
            const bool updatedMesh = updateInputMesh(doc);
            const bool updatedBlendingFactor = updateBlendingFactor(doc);
            const bool updatedRecalculateNormalsState = updateRecalculateNormalsState(doc);

            if (updatedCoefficients || updatedNonParticipatingLandmarks || updatedMesh || updatedBlendingFactor || updatedRecalculateNormalsState)
            {
                m_CachedResultMesh = TPSWarpMesh(m_CachedCoefficients, m_CachedSourceMesh, m_CachedBlendingFactor);
                if (m_CachedRecalculateNormalsState) {
                    m_CachedResultMesh.recalculate_normals();
                }
                m_CachedResultNonParticipatingLandmarks = opyn::TPSWarpPoints(m_CachedCoefficients, m_CachedSourceNonParticipatingLandmarks, m_CachedBlendingFactor);
            }
        }

        // returns `true` if cached inputs were updated; otherwise, returns the cached inputs
        bool updateInputs(const TPSDocument& doc)
        {
            opyn::TPSCoefficientSolverInputs3D newInputs{GetLandmarkPairs(doc)};
            for (auto& [source, destination] : newInputs.landmarks) {
                source *= doc.sourceLandmarksPrescale;
                destination *= doc.destinationLandmarksPrescale;
            }
            newInputs.applyAffineTranslation = doc.applyAffineTranslation;
            newInputs.applyAffineScale = doc.applyAffineScale;
            newInputs.applyAffineRotation = doc.applyAffineRotation;
            newInputs.applyNonAffineWarp = doc.applyNonAffineWarp;

            if (newInputs != m_CachedInputs) {
                m_CachedInputs = std::move(newInputs);
                return true;
            }
            else {
                return false;
            }
        }

        // returns `true` if cached coefficients were updated
        bool updateCoefficients(const TPSDocument& doc)
        {
            if (!updateInputs(doc))
            {
                // cache: the inputs have not been updated, so the coefficients will not change
                return false;
            }

            opyn::TPSCoefficients3D newCoefficients = TPSCalcCoefficients(m_CachedInputs);

            if (newCoefficients != m_CachedCoefficients)
            {
                m_CachedCoefficients = std::move(newCoefficients);
                return true;
            }
            else
            {
                return false;  // no change in the coefficients
            }
        }


        bool updateSourceNonParticipatingLandmarks(const TPSDocument& doc)
        {
            const auto& docLandmarks = doc.nonParticipatingLandmarks;

            const bool samePositions = rgs::equal(
                docLandmarks,
                m_CachedSourceNonParticipatingLandmarks,
                [](const TPSDocumentNonParticipatingLandmark& lm, const SimTK::fVec3& position)
                {
                    return lm.location.x() == position[0] and lm.location.y() == position[1] and lm.location.z() == position[2];
                }
            );

            if (!samePositions)
            {
                m_CachedSourceNonParticipatingLandmarks.clear();
                rgs::transform(
                    docLandmarks,
                    std::back_inserter(m_CachedSourceNonParticipatingLandmarks),
                    [](const auto& lm) { return to<SimTK::fVec3>(lm.location); }
                );
                return true;
            }
            else
            {
                return false;
            }
        }

        // returns `true` if `m_CachedSourceMesh` is updated
        bool updateInputMesh(const TPSDocument& doc)
        {
            if (m_CachedSourceMesh != doc.sourceMesh)
            {
                m_CachedSourceMesh = doc.sourceMesh;
                return true;
            }
            else
            {
                return false;
            }
        }

        bool updateBlendingFactor(const TPSDocument& doc)
        {
            if (m_CachedBlendingFactor != doc.blendingFactor) {
                m_CachedBlendingFactor = doc.blendingFactor;
                return true;
            }
            else {
                return false;
            }
        }

        bool updateRecalculateNormalsState(const TPSDocument& doc)
        {
            if (m_CachedRecalculateNormalsState != doc.recalculateNormals) {
                m_CachedRecalculateNormalsState = doc.recalculateNormals;
                return true;
            }
            else {
                return false;
            }
        }

        opyn::TPSCoefficientSolverInputs3D<float> m_CachedInputs;
        opyn::TPSCoefficients3D<float> m_CachedCoefficients;
        Mesh m_CachedSourceMesh;
        float m_CachedBlendingFactor = 1.0f;
        bool m_CachedRecalculateNormalsState = false;
        Mesh m_CachedResultMesh;
        std::vector<SimTK::fVec3> m_CachedSourceNonParticipatingLandmarks;
        std::vector<SimTK::fVec3> m_CachedResultNonParticipatingLandmarks;
    };
}

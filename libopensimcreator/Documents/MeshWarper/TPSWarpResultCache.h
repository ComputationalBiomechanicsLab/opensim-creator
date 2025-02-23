#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <libopensimcreator/Utils/TPS3D.h>

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vec3.h>

#include <algorithm>
#include <ranges>
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

        std::span<const Vec3> getWarpedNonParticipatingLandmarkLocations(const TPSDocument& doc)
        {
            updateAll(doc);
            return m_CachedResultNonParticipatingLandmarks;
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
                m_CachedResultNonParticipatingLandmarks = TPSWarpPoints(m_CachedCoefficients, m_CachedSourceNonParticipatingLandmarks, m_CachedBlendingFactor);
            }
        }

        // returns `true` if cached inputs were updated; otherwise, returns the cached inputs
        bool updateInputs(const TPSDocument& doc)
        {
            TPSCoefficientSolverInputs3D newInputs
            {
                GetLandmarkPairs(doc),
            };

            if (newInputs != m_CachedInputs)
            {
                m_CachedInputs = std::move(newInputs);
                return true;
            }
            else
            {
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

            TPSCoefficients3D newCoefficients = TPSCalcCoefficients(m_CachedInputs);

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
                [](const TPSDocumentNonParticipatingLandmark& lm, const Vec3& pos)
                {
                    return lm.location == pos;
                }
            );

            if (!samePositions)
            {
                m_CachedSourceNonParticipatingLandmarks.clear();
                rgs::transform(
                    docLandmarks,
                    std::back_inserter(m_CachedSourceNonParticipatingLandmarks),
                    [](const auto& lm) { return lm.location; }
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

        TPSCoefficientSolverInputs3D<float> m_CachedInputs;
        TPSCoefficients3D<float> m_CachedCoefficients;
        Mesh m_CachedSourceMesh;
        float m_CachedBlendingFactor = 1.0f;
        bool m_CachedRecalculateNormalsState = false;
        Mesh m_CachedResultMesh;
        std::vector<Vec3> m_CachedSourceNonParticipatingLandmarks;
        std::vector<Vec3> m_CachedResultNonParticipatingLandmarks;
    };
}

#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Utils/TPS3D.h>

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

namespace osc
{
    // TPS result cache
    //
    // caches the result of an (expensive) TPS warp of the mesh by checking
    // whether the warping parameters have changed
    class TPSResultCache final {
    public:
        Mesh const& getWarpedMesh(TPSDocument const& doc)
        {
            updateAll(doc);
            return m_CachedResultMesh;
        }

        std::span<Vec3 const> getWarpedNonParticipatingLandmarkLocations(TPSDocument const& doc)
        {
            updateAll(doc);
            return m_CachedResultNonParticipatingLandmarks;
        }

    private:
        void updateAll(TPSDocument const& doc)
        {
            bool const updatedCoefficients = updateCoefficients(doc);
            bool const updatedNonParticipatingLandmarks = updateSourceNonParticipatingLandmarks(doc);
            bool const updatedMesh = updateInputMesh(doc);
            bool const updatedBlendingFactor = updateBlendingFactor(doc);

            if (updatedCoefficients || updatedNonParticipatingLandmarks || updatedMesh || updatedBlendingFactor)
            {
                m_CachedResultMesh = ApplyThinPlateWarpToMesh(m_CachedCoefficients, m_CachedSourceMesh, m_CachedBlendingFactor);
                m_CachedResultNonParticipatingLandmarks = ApplyThinPlateWarpToPoints(m_CachedCoefficients, m_CachedSourceNonParticipatingLandmarks, m_CachedBlendingFactor);
            }
        }

        // returns `true` if cached inputs were updated; otherwise, returns the cached inputs
        bool updateInputs(TPSDocument const& doc)
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
        bool updateCoefficients(TPSDocument const& doc)
        {
            if (!updateInputs(doc))
            {
                // cache: the inputs have not been updated, so the coefficients will not change
                return false;
            }

            TPSCoefficients3D newCoefficients = CalcCoefficients(m_CachedInputs);

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


        bool updateSourceNonParticipatingLandmarks(TPSDocument const& doc)
        {
            auto const& docLandmarks = doc.nonParticipatingLandmarks;

            bool const samePositions = equal(
                docLandmarks,
                m_CachedSourceNonParticipatingLandmarks,
                [](TPSDocumentNonParticipatingLandmark const& lm, Vec3 const& pos)
                {
                    return lm.location == pos;
                }
            );

            if (!samePositions)
            {
                m_CachedSourceNonParticipatingLandmarks.clear();
                std::transform(
                    docLandmarks.begin(),
                    docLandmarks.end(),
                    std::back_inserter(m_CachedSourceNonParticipatingLandmarks),
                    [](auto const& lm) { return lm.location; }
                );
                return true;
            }
            else
            {
                return false;
            }
        }

        // returns `true` if `m_CachedSourceMesh` is updated
        bool updateInputMesh(TPSDocument const& doc)
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

        bool updateBlendingFactor(TPSDocument const& doc)
        {
            if (m_CachedBlendingFactor != doc.blendingFactor) {
                m_CachedBlendingFactor = doc.blendingFactor;
                return true;
            }
            else {
                return false;
            }
        }

        TPSCoefficientSolverInputs3D m_CachedInputs;
        TPSCoefficients3D m_CachedCoefficients;
        Mesh m_CachedSourceMesh;
        float m_CachedBlendingFactor = 1.0f;
        Mesh m_CachedResultMesh;
        std::vector<Vec3> m_CachedSourceNonParticipatingLandmarks;
        std::vector<Vec3> m_CachedResultNonParticipatingLandmarks;
    };
}

#pragma once

#include <libopynsim/documents/custom_components/in_memory_mesh.h>
#include <libopynsim/documents/landmarks/landmark_helpers.h>
#include <libopynsim/documents/landmarks/maybe_named_landmark_pair.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_common_inputs.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <libopynsim/tps3d.h>

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/conversion.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon/internal/Transform.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/SmallMatrix.h>

#include <memory>
#include <utility>

namespace opyn
{
    // A cache that is (presumed to be) persisted between multiple executions of the
    // model warping pipeline, in order to improve runtime performance.
    class ScalingCache final {
    public:
        std::unique_ptr<InMemoryMesh> lookupTPSMeshWarp(
            const OpenSim::Model& sourceModel,
            const OpenSim::Model& resultModel,
            const OpenSim::Mesh& sourceMesh,
            const OpenSim::Mesh& resultMesh,
            const OpenSim::Frame& sourceLandmarksFrame,
            const OpenSim::Frame& resultLandmarksFrame,
            const ThinPlateSplineCommonInputs& tpsInputs,
            bool compensateForFrameChanges)
        {
            // Compile the TPS coefficients from the source+destination landmarks
            const TPSCoefficients3D<float>& coefficients = lookupTPSCoefficients(tpsInputs);

            // Calculate transforms to use before/after TPS warping
            const Transforms transforms = calculateTransforms(
                sourceModel,
                resultModel,
                sourceMesh.getFrame(),
                resultMesh.getFrame(),
                sourceLandmarksFrame,
                resultLandmarksFrame,
                compensateForFrameChanges
            );

            // Convert the input mesh into an OSC mesh, so that it's suitable for warping.
            osc::Mesh resultOscMesh = ToOscMesh(
                resultModel,
                resultModel.getWorkingState(),
                resultMesh
            );

            // Warp the vertices in-place.
            auto vertices = resultOscMesh.vertices();
            for (auto& vertex : vertices) {
                vertex = transform_point(transforms.localToLandmarks, vertex);  // put vertex into landmark frame
            }
            static_assert(alignof(decltype(vertices.front())) == alignof(SimTK::fVec3));
            static_assert(sizeof(decltype(vertices.front())) == sizeof(SimTK::fVec3));
            auto* punned = std::launder(reinterpret_cast<SimTK::fVec3*>(vertices.data()));
            tps3d_warp_points_in_place(coefficients, {punned, vertices.size()}, static_cast<float>(tpsInputs.blendingFactor));
            for (auto& vertex : vertices) {
                vertex = transform_point(transforms.landmarksToLocal, vertex);  // put vertex back into mesh frame
            }

            // Assign the vertices back to the OSC mesh and emit it as an `InMemoryMesh` component
            resultOscMesh.set_vertices(vertices);
            resultOscMesh.recalculate_normals();  // maybe should be a runtime param
            return std::make_unique<InMemoryMesh>(resultOscMesh);
        }

        SimTK::Vec3 lookupTPSWarpedRigidPoint(
            const OpenSim::Model& sourceModel,
            const OpenSim::Model& resultModel,
            [[maybe_unused]] const SimTK::Vec3& sourceLocation,
            const SimTK::Vec3& resultLocation,
            const OpenSim::Frame& sourceParentFrame,
            const OpenSim::Frame& resultParentFrame,
            const OpenSim::Frame& sourceLandmarksFrame,
            const OpenSim::Frame& resultLandmarksFrame,
            const ThinPlateSplineCommonInputs& tpsInputs,
            bool compensateForFrameChanges)
        {
            // Compile the TPS coefficients from the source+destination landmarks
            const TPSCoefficients3D<float>& coefficients = lookupTPSCoefficients(tpsInputs);

            // Calculate transforms to use before/after TPS warping
            const Transforms transforms = calculateTransforms(
                sourceModel,
                resultModel,
                sourceParentFrame,
                resultParentFrame,
                sourceLandmarksFrame,
                resultLandmarksFrame,
                compensateForFrameChanges
            );

            const auto resultLocationInLandmarkFrame = transform_point(transforms.localToLandmarks, osc::to<osc::Vector3>(resultLocation));
            const auto warpedLocationInLandmarkFrame = tps3d_warp_point(coefficients, osc::to<SimTK::fVec3>(resultLocationInLandmarkFrame), static_cast<float>(tpsInputs.blendingFactor));
            return osc::to<SimTK::Vec3>(osc::transform_point(transforms.landmarksToLocal, osc::to<osc::Vector3>(warpedLocationInLandmarkFrame)));
        }

        SimTK::Transform lookupTPSAffineTransformWithoutScaling(
            const ThinPlateSplineCommonInputs& tpsInputs)
        {
            OSC_ASSERT_ALWAYS(tpsInputs.applyAffineRotation && "affine rotation must be requested in order to figure out the transform");
            OSC_ASSERT_ALWAYS(tpsInputs.applyAffineTranslation && "affine translation must be requested in order to figure out the transform");
            const TPSCoefficients3D<float>& coefficients = lookupTPSCoefficients(tpsInputs);

            const SimTK::Vec<3, float> x = coefficients.a2.normalize();
            const SimTK::Vec<3, float> y = coefficients.a3.normalize();
            const SimTK::Vec<3, float> z = coefficients.a4.normalize();
            const SimTK::Mat33 rotationMatrix{
                x[0], y[0], z[0],
                x[1], y[1], z[1],
                x[2], y[2], z[2],
            };
            const SimTK::Rotation rotation{rotationMatrix};
            const auto translation = osc::to<SimTK::Vec3>(coefficients.a1);
            return SimTK::Transform{rotation, translation};
        }
    private:
        struct Transforms final {
            osc::Matrix4x4 localToLandmarks;
            osc::Matrix4x4 landmarksToLocal;
        };

        Transforms calculateTransforms(
            const OpenSim::Model& sourceModel,
            const OpenSim::Model& resultModel,
            const OpenSim::Frame& sourceFrame,
            const OpenSim::Frame& resultFrame,
            const OpenSim::Frame& sourceLandmarksFrame,
            const OpenSim::Frame& resultLandmarksFrame,
            bool compensateForFrameChanges) const
        {
            const SimTK::Transform resultTransform = resultFrame.findTransformBetween(resultModel.getWorkingState(), resultLandmarksFrame);

            if (compensateForFrameChanges) {
                const SimTK::Transform sourceTransform = sourceFrame.findTransformBetween(sourceModel.getWorkingState(), sourceLandmarksFrame);
                const SimTK::Transform frameWarpTransform = sourceTransform.invert() * resultTransform;
                return Transforms{
                    .localToLandmarks = osc::to<osc::Matrix4x4>(sourceTransform),
                    .landmarksToLocal = osc::to<osc::Matrix4x4>(frameWarpTransform.invert() * sourceTransform.invert()),
                };
            } else {
                return Transforms{
                    .localToLandmarks = osc::to<osc::Matrix4x4>(resultTransform),
                    .landmarksToLocal = osc::to<osc::Matrix4x4>(SimTK::Transform{resultTransform.invert()}),
                };
            }
        }

        const TPSCoefficients3D<float>& lookupTPSCoefficients(const ThinPlateSplineCommonInputs& tpsInputs)
        {
            // Read source+destination landmark files into independent collections
            const auto sourceLandmarks = ReadLandmarksFromCSVIntoVectorOrThrow(tpsInputs.sourceLandmarksPath);
            const auto destinationLandmarks = ReadLandmarksFromCSVIntoVectorOrThrow(tpsInputs.destinationLandmarksPath);

            // Pair the source+destination landmarks together into a TPS coefficient solver's inputs
            TPSCoefficientSolverInputs3D<float> inputs;
            inputs.landmarks.reserve(osc::max(sourceLandmarks.size(), destinationLandmarks.size()));
            TryPairingLandmarks(sourceLandmarks, destinationLandmarks, [&inputs, &tpsInputs](const MaybeNamedLandmarkPair& p)
            {
                if (auto landmark3d = p.tryGetPairedLocations()) {
                    landmark3d->source = tpsInputs.sourceLandmarksPrescale * landmark3d->source;
                    landmark3d->destination = tpsInputs.destinationLandmarksPrescale * landmark3d->destination;
                    inputs.landmarks.push_back(*landmark3d);
                }
                else {
                    osc::log_warn("The landmarks %s could not be paired, might be missing in the source/destination?", p.name().c_str());
                }
            });
            inputs.apply_affine_translation = tpsInputs.applyAffineTranslation;
            inputs.apply_affine_scale = tpsInputs.applyAffineScale;
            inputs.apply_affine_rotation = tpsInputs.applyAffineRotation;
            inputs.apply_non_affine_warp = tpsInputs.applyNonAffineWarp;

            // Solve the coefficients
            m_CoefficientsTODO = opyn::tps3d_solve_coefficients(inputs);

            return m_CoefficientsTODO;
        }

        TPSCoefficients3D<float> m_CoefficientsTODO;
    };
}

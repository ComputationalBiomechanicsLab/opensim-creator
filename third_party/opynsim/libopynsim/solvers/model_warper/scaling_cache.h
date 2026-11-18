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
#include <unordered_map>
#include <utility>

namespace opyn
{
    // A cache that is (presumed to be) persisted between multiple executions of the
    // model warping pipeline, in order to improve runtime performance.
    class ScalingCache final {
    public:
        std::unique_ptr<InMemoryMesh> lookup_tps_mesh_warp(
            const OpenSim::Model& source_model,
            const OpenSim::Model& result_model,
            const OpenSim::Mesh& source_mesh,
            const OpenSim::Mesh& result_mesh,
            const OpenSim::Frame& source_landmarks_frame,
            const OpenSim::Frame& result_landmarks_frame,
            const ThinPlateSplineCommonInputs& tps_inputs,
            bool compensate_for_frame_changes)
        {
            // Compile the TPS coefficients from the source+destination landmarks
            const TPSCoefficients3D<float>& coefficients = lookup_tps_coefficients(tps_inputs);

            // Calculate transforms to use before/after TPS warping
            const Transforms transforms = calculate_transforms(
                source_model,
                result_model,
                source_mesh.getFrame(),
                result_mesh.getFrame(),
                source_landmarks_frame,
                result_landmarks_frame,
                compensate_for_frame_changes);

            // Convert the input mesh into an OSC mesh, so that it's suitable for warping.
            osc::Mesh result_osc_mesh =
                to_osc_mesh(result_model, result_model.getWorkingState(), result_mesh);

            // Warp the vertices in-place.
            auto vertices = result_osc_mesh.vertices();
            for (auto& vertex : vertices) {
                // put vertex into landmark frame
                vertex = transform_point(transforms.local_to_landmarks, vertex);
            }
            static_assert(alignof(decltype(vertices.front())) == alignof(SimTK::fVec3));
            static_assert(sizeof(decltype(vertices.front())) == sizeof(SimTK::fVec3));
            auto* punned = std::launder(reinterpret_cast<SimTK::fVec3*>(vertices.data()));
            tps3d_warp_points_in_place(
                coefficients,
                {punned, vertices.size()},
                static_cast<float>(tps_inputs.blending_factor));
            for (auto& vertex : vertices) {
                // put vertex back into mesh frame
                vertex = transform_point(transforms.landmarks_to_local, vertex);
            }

            // Assign the vertices back to the OSC mesh and emit it as an `InMemoryMesh` component
            result_osc_mesh.set_vertices(vertices);
            result_osc_mesh.recalculate_normals();  // maybe should be a runtime param
            return std::make_unique<InMemoryMesh>(result_osc_mesh);
        }

        SimTK::Vec3 lookup_tps_warped_rigid_point(
            const OpenSim::Model& source_model,
            const OpenSim::Model& result_model,
            [[maybe_unused]] const SimTK::Vec3& source_location,
            const SimTK::Vec3& result_location,
            const OpenSim::Frame& source_parent_frame,
            const OpenSim::Frame& result_parent_frame,
            const OpenSim::Frame& source_landmarks_frame,
            const OpenSim::Frame& result_landmarks_frame,
            const ThinPlateSplineCommonInputs& tps_inputs,
            bool compensate_for_frame_changes)
        {
            // Compile the TPS coefficients from the source+destination landmarks
            const TPSCoefficients3D<float>& coefficients = lookup_tps_coefficients(tps_inputs);

            // Calculate transforms to use before/after TPS warping
            const Transforms transforms = calculate_transforms(
                source_model,
                result_model,
                source_parent_frame,
                result_parent_frame,
                source_landmarks_frame,
                result_landmarks_frame,
                compensate_for_frame_changes);

            const auto result_location_in_landmark_frame = transform_point(
                transforms.local_to_landmarks, osc::to<osc::Vector3>(result_location));
            const auto warped_location_in_landmark_frame = tps3d_warp_point(
                coefficients,
                osc::to<SimTK::fVec3>(result_location_in_landmark_frame),
                static_cast<float>(tps_inputs.blending_factor));
            return osc::to<SimTK::Vec3>(osc::transform_point(
                transforms.landmarks_to_local,
                osc::to<osc::Vector3>(warped_location_in_landmark_frame)));
        }

        SimTK::Transform lookup_tps_affine_transform_without_scaling(
            const ThinPlateSplineCommonInputs& tps_inputs)
        {
            OSC_ASSERT_ALWAYS(
                tps_inputs.apply_affine_rotation &&
                "affine rotation must be requested in order to figure out the transform");
            OSC_ASSERT_ALWAYS(
                tps_inputs.apply_affine_translation &&
                "affine translation must be requested in order to figure out the transform");
            const TPSCoefficients3D<float>& coefficients = lookup_tps_coefficients(tps_inputs);

            const SimTK::Vec<3, float> x = coefficients.a2.normalize();
            const SimTK::Vec<3, float> y = coefficients.a3.normalize();
            const SimTK::Vec<3, float> z = coefficients.a4.normalize();
            const SimTK::Mat33 rotation_matrix{
                x[0],
                y[0],
                z[0],
                x[1],
                y[1],
                z[1],
                x[2],
                y[2],
                z[2],
            };
            const SimTK::Rotation rotation{rotation_matrix};
            const auto translation = osc::to<SimTK::Vec3>(coefficients.a1);
            return SimTK::Transform{rotation, translation};
        }

        void clear() { coefficient_cache_.clear(); }

    private:
        struct Transforms final {
            osc::Matrix4x4 local_to_landmarks;
            osc::Matrix4x4 landmarks_to_local;
        };

        Transforms calculate_transforms(
            const OpenSim::Model& source_model,
            const OpenSim::Model& result_model,
            const OpenSim::Frame& source_frame,
            const OpenSim::Frame& result_frame,
            const OpenSim::Frame& source_landmarks_frame,
            const OpenSim::Frame& result_landmarks_frame,
            bool compensate_for_frame_changes) const
        {
            const SimTK::Transform result_transform = result_frame.findTransformBetween(
                result_model.getWorkingState(), result_landmarks_frame);

            if (compensate_for_frame_changes) {
                const SimTK::Transform source_transform = source_frame.findTransformBetween(
                    source_model.getWorkingState(), source_landmarks_frame);
                const SimTK::Transform frame_warp_transform =
                    source_transform.invert() * result_transform;
                return Transforms{
                    .local_to_landmarks = osc::to<osc::Matrix4x4>(source_transform),
                    .landmarks_to_local = osc::to<osc::Matrix4x4>(
                        frame_warp_transform.invert() * source_transform.invert()),
                };
            } else {
                return Transforms{
                    .local_to_landmarks = osc::to<osc::Matrix4x4>(result_transform),
                    .landmarks_to_local =
                        osc::to<osc::Matrix4x4>(SimTK::Transform{result_transform.invert()}),
                };
            }
        }

        const TPSCoefficients3D<float>& lookup_tps_coefficients(
            const ThinPlateSplineCommonInputs& tps_inputs)
        {
            const auto [cacheEntry, inserted] = coefficient_cache_.try_emplace(tps_inputs);
            if (not inserted) {
                return cacheEntry->second;
            }

            // Read source+destination landmark files into independent collections
            const auto source_landmarks =
                read_landmarks_from_csv_into_vector_or_throw(tps_inputs.source_landmarks_path);
            const auto destination_landmarks =
                read_landmarks_from_csv_into_vector_or_throw(tps_inputs.destination_landmarks_path);

            // Pair the source+destination landmarks together into a TPS coefficient solver's inputs
            TPSCoefficientSolverInputs3D<float> inputs;
            inputs.landmarks.reserve(
                osc::max(source_landmarks.size(), destination_landmarks.size()));
            try_pairing_landmarks(
                source_landmarks,
                destination_landmarks,
                [&inputs, &tps_inputs](const MaybeNamedLandmarkPair& p) {
                    if (auto landmark3d = p.try_get_paired_locations()) {
                        landmark3d->source =
                            tps_inputs.source_landmarks_prescale * landmark3d->source;
                        landmark3d->destination =
                            tps_inputs.destination_landmarks_prescale * landmark3d->destination;
                        inputs.landmarks.push_back(*landmark3d);
                    } else {
                        osc::log_warn(
                            "The landmarks {} could not be paired, might be missing in the source/destination?",
                            p.name());
                    }
                });
            inputs.apply_affine_translation = tps_inputs.apply_affine_translation;
            inputs.apply_affine_scale = tps_inputs.apply_affine_scale;
            inputs.apply_affine_rotation = tps_inputs.apply_affine_rotation;
            inputs.apply_non_affine_warp = tps_inputs.apply_non_affine_warp;
            inputs.warping_penalty = static_cast<float>(tps_inputs.warping_penalty);

            // Solve the coefficients
            cacheEntry->second = opyn::tps3d_solve_coefficients(inputs);

            return cacheEntry->second;
        }

        std::unordered_map<ThinPlateSplineCommonInputs, TPSCoefficients3D<float>>
            coefficient_cache_;
    };
}

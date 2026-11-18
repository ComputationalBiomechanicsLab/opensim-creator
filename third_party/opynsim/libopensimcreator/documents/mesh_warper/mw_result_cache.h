#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_helpers.h>

#include <libopynsim/utilities/simbody_x_oscar.h>
#include <libopynsim/tps3d.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

namespace rgs = std::ranges;

namespace osc
{
    /// A class that caches mesh warping computation (e.g. TPS coefficient
    /// derivation and mesh warping) so that the mesh warping UI can operate
    /// more quickly between user edits.
    class MwResultCache final {
    public:
        const Mesh& get_warped_mesh(const MwDocument& doc)
        {
            update_all(doc);
            return cached_result_mesh_;
        }

        std::span<const Vector3> get_warped_non_participating_landmark_locations(
            const MwDocument& doc)
        {
            update_all(doc);
            static_assert(
                alignof(decltype(cached_result_non_participating_landmarks_.front())) ==
                alignof(SimTK::fVec3));
            static_assert(
                sizeof(decltype(cached_result_non_participating_landmarks_.front())) ==
                sizeof(SimTK::fVec3));
            const auto* ptr = std::launder(
                reinterpret_cast<const Vector3*>(
                    cached_result_non_participating_landmarks_.data()));
            return {ptr, cached_result_non_participating_landmarks_.size()};
        }

    private:
        void update_all(const MwDocument& doc)
        {
            const bool updated_coefficients = update_coefficients(doc);
            const bool updated_non_participating_landmarks =
                update_source_non_participating_landmarks(doc);
            const bool updated_mesh = update_input_mesh(doc);
            const bool updated_blending_factor = update_blending_factor(doc);
            const bool updated_recalculate_normals_state = update_recalculate_normals_state(doc);

            if (updated_coefficients or                 //
                updated_non_participating_landmarks or  //
                updated_mesh or                         //
                updated_blending_factor or              //
                updated_recalculate_normals_state) {

                cached_result_mesh_ = tps3d_warp_mesh(
                    cached_coefficients_, cached_source_mesh_, cached_blending_factor_);
                if (cached_recalculate_normals_state_) {
                    cached_result_mesh_.recalculate_normals();
                }
                cached_result_non_participating_landmarks_ = opyn::tps3d_warp_points(
                    cached_coefficients_,
                    cached_source_non_participating_landmarks_,
                    cached_blending_factor_);
            }
        }

        // returns `true` if cached inputs were updated; otherwise, returns the cached inputs
        bool update_inputs(const MwDocument& doc)
        {
            opyn::TPSCoefficientSolverInputs3D new_inputs{get_landmark_pairs(doc)};
            for (auto& [source, destination] : new_inputs.landmarks) {
                source *= doc.source_landmarks_prescale;
                destination *= doc.destination_landmarks_prescale;
            }
            new_inputs.warping_penalty = doc.warping_penalty;
            new_inputs.apply_affine_translation = doc.apply_affine_translation;
            new_inputs.apply_affine_scale = doc.apply_affine_scale;
            new_inputs.apply_affine_rotation = doc.apply_affine_rotation;
            new_inputs.apply_non_affine_warp = doc.apply_non_affine_warp;

            if (new_inputs != cached_inputs_) {
                cached_inputs_ = std::move(new_inputs);
                return true;
            } else {
                return false;
            }
        }

        // returns `true` if cached coefficients were updated
        bool update_coefficients(const MwDocument& doc)
        {
            if (!update_inputs(doc)) {
                // cache: the inputs have not been updated, so the coefficients will not change
                return false;
            }

            opyn::TPSCoefficients3D new_coefficients = tps3d_solve_coefficients(cached_inputs_);

            if (new_coefficients != cached_coefficients_) {
                cached_coefficients_ = std::move(new_coefficients);
                return true;
            } else {
                return false;  // no change in the coefficients
            }
        }

        bool update_source_non_participating_landmarks(const MwDocument& doc)
        {
            const auto& doc_landmarks = doc.non_participating_landmarks;

            const auto same_position = [](const MwDocumentNonParticipatingLandmark& lm,
                                          const SimTK::fVec3& position) {
                return lm.location.x() == position[0] and  //
                       lm.location.y() == position[1] and  //
                       lm.location.z() == position[2];
            };
            const bool same_positions = rgs::equal(
                doc_landmarks, cached_source_non_participating_landmarks_, same_position);

            if (!same_positions) {
                cached_source_non_participating_landmarks_.clear();
                rgs::transform(
                    doc_landmarks,
                    std::back_inserter(cached_source_non_participating_landmarks_),
                    [](const auto& lm) { return to<SimTK::fVec3>(lm.location); });
                return true;
            } else {
                return false;
            }
        }

        // returns `true` if `m_CachedSourceMesh` is updated
        bool update_input_mesh(const MwDocument& doc)
        {
            if (cached_source_mesh_ != doc.source_mesh) {
                cached_source_mesh_ = doc.source_mesh;
                return true;
            } else {
                return false;
            }
        }

        bool update_blending_factor(const MwDocument& doc)
        {
            if (cached_blending_factor_ != doc.blending_factor) {
                cached_blending_factor_ = doc.blending_factor;
                return true;
            } else {
                return false;
            }
        }

        bool update_recalculate_normals_state(const MwDocument& doc)
        {
            if (cached_recalculate_normals_state_ != doc.recalculate_normals) {
                cached_recalculate_normals_state_ = doc.recalculate_normals;
                return true;
            } else {
                return false;
            }
        }

        opyn::TPSCoefficientSolverInputs3D<float> cached_inputs_;
        opyn::TPSCoefficients3D<float> cached_coefficients_;
        Mesh cached_source_mesh_;
        float cached_blending_factor_ = 1.0f;
        bool cached_recalculate_normals_state_ = false;
        Mesh cached_result_mesh_;
        std::vector<SimTK::fVec3> cached_source_non_participating_landmarks_;
        std::vector<SimTK::fVec3> cached_result_non_participating_landmarks_;
    };
}

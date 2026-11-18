#pragma once

#include <libopynsim/solvers/model_warper/toggleable_thin_plate_spline_scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <SimTKcommon/SmallMatrix.h>

#include <format>
#include <string>
#include <vector>

namespace opyn
{
    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp to any
    // `OpenSim::PathPoint`s it can find via the `path_points` search string.
    class ThinPlateSplinePathPointsScalingStep final : public ToggleableThinPlateSplineScalingStep {

        OpenSim_DECLARE_CONCRETE_OBJECT(
            ThinPlateSplinePathPointsScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(
            path_points,
            std::string,
            "Query paths (e.g. `/forceset/*`) that the engine should use to find path points in the source model that should be warped by this scaling step.");

    public:
        explicit ThinPlateSplinePathPointsScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Path Points"}
        {
            setDescription(
                "Warps the locations of path points in the model using the Thin-Plate Spline (TPS) warping algorithm.");
            constructProperty_path_points();
        }

    private:
        std::vector<ScalingStepValidationMessage> impl_validate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& source_model) const override
        {
            // Get base class validation messages.
            auto messages =
                ToggleableThinPlateSplineScalingStep::impl_validate(cache, params, source_model);

            // Ensure every entry in `path_points` can be found in the source model.
            for (int i = 0; i < getProperty_path_points().size(); ++i) {
                const auto* path_point =
                    find_component<OpenSim::PathPoint>(source_model, get_path_points(i));
                if (not path_point) {
                    messages.emplace_back(
                        ScalingStepValidationState::Error,
                        std::format(
                            "{}: Cannot find a PathPoint in 'path_points' in the source model (or it isn't a PathPoint)",
                            get_path_points(i)));
                }
            }

            return messages;
        }

        void impl_apply_scaling_step(
            ScalingCache& scaling_cache,
            const ScalingParameters& parameters,
            const OpenSim::Model& source_model,
            OpenSim::Model& result_model) const override
        {
            const auto common_params =
                calc_tps_scaling_step_common_params(parameters, source_model, result_model);

            // Warp each path point specified by the `path_points` property.
            for (int i = 0; i < getProperty_path_points().size(); ++i) {
                const auto* source_path_point =
                    find_component<OpenSim::PathPoint>(source_model, get_path_points(i));
                OSC_ASSERT_ALWAYS(
                    source_path_point && "could not find a path point in the source model");

                // Find the path point in the source model and use it produce the warped path point.
                const auto* result_path_point =
                    find_component<OpenSim::PathPoint>(result_model, get_path_points(i));
                OSC_ASSERT_ALWAYS(result_path_point && "could not find a path point in the model");

                const SimTK::Vec3 warped_location = scaling_cache.lookup_tps_warped_rigid_point(
                    source_model,
                    result_model,
                    source_path_point->get_location(),
                    result_path_point->get_location(),
                    source_path_point->getParentFrame(),
                    result_path_point->getParentFrame(),
                    *common_params.source_landmarks_frame,
                    *common_params.result_landmarks_frame,
                    common_params.tps_inputs,
                    common_params.compensate_for_frame_changes);

                auto* result_path_point_mut =
                    find_component_mut<OpenSim::PathPoint>(result_model, get_path_points(i));
                OSC_ASSERT_ALWAYS(
                    result_path_point_mut &&
                    "could not find a corresponding path point in the result model");
                result_path_point_mut->set_location(warped_location);
            }
            initialize_model(result_model);
            initialize_state(result_model);
        }
    };
}

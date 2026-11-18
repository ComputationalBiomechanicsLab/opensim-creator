#pragma once

#include <libopynsim/solvers/model_warper/toggleable_thin_plate_spline_scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <SimTKcommon/SmallMatrix.h>

#include <format>
#include <string>
#include <vector>

namespace OpenSim
{
    class Model;
}

namespace opyn
{
    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp to the
    // `translation` property of an `OpenSim::PhysicalOffsetFrame`.
    class ThinPlateSplineOffsetFrameTranslationScalingStep final
        : public ToggleableThinPlateSplineScalingStep {

        OpenSim_DECLARE_CONCRETE_OBJECT(
            ThinPlateSplineOffsetFrameTranslationScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(
            offset_frames,
            std::string,
            "Absolute paths (e.g. `/jointset/joint/parent_frame`) that the engine should use to find the offset frames in the source model.");

    public:
        explicit ThinPlateSplineOffsetFrameTranslationScalingStep() :
            ToggleableThinPlateSplineScalingStep{
                "Apply Thin-Plate Spline (TPS) to Offset Frame translation"}
        {
            setDescription(
                "Uses the Thin-Plate Spline (TPS) warping algorithm to warp the translation of the given offset frames.");
            constructProperty_offset_frames();
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

            // Ensure every entry in `offset_frames` can be found in the source model.
            for (int i = 0; i < getProperty_offset_frames().size(); ++i) {
                const auto* offset_frame = find_component<OpenSim::PhysicalOffsetFrame>(
                    source_model, get_offset_frames(i));
                if (not offset_frame) {
                    messages.emplace_back(
                        ScalingStepValidationState::Error,
                        std::format(
                            "{}: Cannot find a `PhysicalOffsetFrame` in 'offset_frames' in the source model (or it isn't a `PhysicalOffsetFrame`).",
                            get_offset_frames(i)));
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
            // Lookup/validate warping inputs.
            const auto common_params =
                calc_tps_scaling_step_common_params(parameters, source_model, result_model);

            // Warp each offset frame `translation` specified by the `offset_frames` property.
            for (int i = 0; i < getProperty_offset_frames().size(); ++i) {
                const auto* source_offset_frame = find_component<OpenSim::PhysicalOffsetFrame>(
                    source_model, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(
                    source_offset_frame &&
                    "could not find a `PhysicalOffsetFrame` in the source model");

                // Find the path point in the source model and use it produce the warped path point.
                const auto* result_offset_frame = find_component<OpenSim::PhysicalOffsetFrame>(
                    result_model, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(
                    result_offset_frame && "could not find a `PhysicalOffsetFrame` in the model");

                const SimTK::Vec3 warped_location = scaling_cache.lookup_tps_warped_rigid_point(
                    source_model,
                    result_model,
                    source_offset_frame->get_translation(),
                    result_offset_frame->get_translation(),
                    source_offset_frame->getParentFrame(),
                    result_offset_frame->getParentFrame(),
                    *common_params.source_landmarks_frame,
                    *common_params.result_landmarks_frame,
                    common_params.tps_inputs,
                    common_params.compensate_for_frame_changes);

                auto* result_offset_frame_mut = find_component_mut<OpenSim::PhysicalOffsetFrame>(
                    result_model, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(
                    result_offset_frame_mut &&
                    "could not find a corresponding `PhysicalOffsetFrame` in the result model");
                result_offset_frame_mut->set_translation(warped_location);
            }
            initialize_model(result_model);
            initialize_state(result_model);
        }
    };
}

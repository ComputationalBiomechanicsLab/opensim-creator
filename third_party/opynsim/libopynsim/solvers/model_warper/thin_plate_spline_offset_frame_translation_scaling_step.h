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

#include <sstream>
#include <string>
#include <vector>

namespace OpenSim { class Model; }

namespace opyn
{
    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp to the
    // `translation` property of an `OpenSim::PhysicalOffsetFrame`.
    class ThinPlateSplineOffsetFrameTranslationScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineOffsetFrameTranslationScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(offset_frames, std::string, "Absolute paths (e.g. `/jointset/joint/parent_frame`) that the engine should use to find the offset frames in the source model.");

    public:
        explicit ThinPlateSplineOffsetFrameTranslationScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Offset Frame translation"}
        {
            setDescription("Uses the Thin-Plate Spline (TPS) warping algorithm to warp the translation of the given offset frames.");
            constructProperty_offset_frames();
        }

    private:

        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure every entry in `offset_frames` can be found in the source model.
            for (int i = 0; i < getProperty_offset_frames().size(); ++i) {
                const auto* offsetFrame = FindComponent<OpenSim::PhysicalOffsetFrame>(sourceModel, get_offset_frames(i));
                if (not offsetFrame) {
                    std::stringstream msg;
                    msg << get_offset_frames(i) << ": Cannot find a `PhysicalOffsetFrame` in 'offset_frames' in the source model (or it isn't a `PhysicalOffsetFrame`).";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
                }
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& scalingCache,
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel,
            OpenSim::Model& resultModel) const final
        {
            // Lookup/validate warping inputs.
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel, resultModel);

            // Warp each offset frame `translation` specified by the `offset_frames` property.
            for (int i = 0; i < getProperty_offset_frames().size(); ++i) {
                const auto* sourceOffsetFrame = FindComponent<OpenSim::PhysicalOffsetFrame>(sourceModel, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(sourceOffsetFrame && "could not find a `PhysicalOffsetFrame` in the source model");

                // Find the path point in the source model and use it produce the warped path point.
                const auto* resultOffsetFrame = FindComponent<OpenSim::PhysicalOffsetFrame>(resultModel, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(resultOffsetFrame && "could not find a `PhysicalOffsetFrame` in the model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    resultModel,
                    sourceOffsetFrame->get_translation(),
                    resultOffsetFrame->get_translation(),
                    sourceOffsetFrame->getParentFrame(),
                    resultOffsetFrame->getParentFrame(),
                    *commonParams.sourceLandmarksFrame,
                    *commonParams.resultLandmarksFrame,
                    commonParams.tpsInputs,
                    commonParams.compensateForFrameChanges
                );

                auto* resultOffsetFrameMut = FindComponentMut<OpenSim::PhysicalOffsetFrame>(resultModel, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(resultOffsetFrameMut && "could not find a corresponding `PhysicalOffsetFrame` in the result model");
                resultOffsetFrameMut->set_translation(warpedLocation);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };
}

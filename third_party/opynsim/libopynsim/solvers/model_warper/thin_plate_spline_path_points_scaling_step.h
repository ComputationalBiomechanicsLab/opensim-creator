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
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplinePathPointsScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(path_points, std::string, "Query paths (e.g. `/forceset/*`) that the engine should use to find path points in the source model that should be warped by this scaling step.");

    public:
        explicit ThinPlateSplinePathPointsScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Path Points"}
        {
            setDescription("Warps the locations of path points in the model using the Thin-Plate Spline (TPS) warping algorithm.");
            constructProperty_path_points();
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure every entry in `path_points` can be found in the source model.
            for (int i = 0; i < getProperty_path_points().size(); ++i) {
                const auto* pathPoint = FindComponent<OpenSim::PathPoint>(sourceModel, get_path_points(i));
                if (not pathPoint) {
                    messages.emplace_back(
                        ScalingStepValidationState::Error,
                        std::format("{}: Cannot find a PathPoint in 'path_points' in the source model (or it isn't a PathPoint)", get_path_points(i))
                    );
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
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel, resultModel);

            // Warp each path point specified by the `path_points` property.
            for (int i = 0; i < getProperty_path_points().size(); ++i) {
                const auto* sourcePathPoint = FindComponent<OpenSim::PathPoint>(sourceModel, get_path_points(i));
                OSC_ASSERT_ALWAYS(sourcePathPoint && "could not find a path point in the source model");

                // Find the path point in the source model and use it produce the warped path point.
                const auto* resultPathPoint = FindComponent<OpenSim::PathPoint>(resultModel, get_path_points(i));
                OSC_ASSERT_ALWAYS(resultPathPoint && "could not find a path point in the model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    resultModel,
                    sourcePathPoint->get_location(),
                    resultPathPoint->get_location(),
                    sourcePathPoint->getParentFrame(),
                    resultPathPoint->getParentFrame(),
                    *commonParams.sourceLandmarksFrame,
                    *commonParams.resultLandmarksFrame,
                    commonParams.tpsInputs,
                    commonParams.compensateForFrameChanges
                );

                auto* resultPathPointMut = FindComponentMut<OpenSim::PathPoint>(resultModel, get_path_points(i));
                OSC_ASSERT_ALWAYS(resultPathPointMut && "could not find a corresponding path point in the result model");
                resultPathPointMut->set_location(warpedLocation);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };
}

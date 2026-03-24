#pragma once

#include <libopynsim/solvers/model_warper/toggleable_thin_plate_spline_scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <SimTKcommon/SmallMatrix.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Model/Station.h>

#include <sstream>
#include <string>

namespace OpenSim { class Model; }

namespace opyn
{
    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp to any
    // `OpenSim::Station`s it can find via the `stations` search string.
    class ThinPlateSplineStationsScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineStationsScalingStep, ThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(stations, std::string, "Query paths (e.g. `/forceset/*`) that the engine should use to find stations in the source model that should be warped by this scaling step.");

    public:
        explicit ThinPlateSplineStationsScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Stations"}
        {
            setDescription("Warps the locations of stations in the model using the Thin-Plate Spline (TPS) warping algorithm.");
            constructProperty_stations();
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure every entry in `stations` can be found in the source model.
            for (int i = 0; i < getProperty_stations().size(); ++i) {
                const auto* station = FindComponent<OpenSim::Station>(sourceModel, get_stations(i));
                if (not station) {
                    std::stringstream msg;
                    msg << get_stations(i) << ": Cannot a Station in 'stations' in the source model (or it isn't a Station).";
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
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel, resultModel);

            // Warp each station specified by the `stations` property.
            for (int i = 0; i < getProperty_stations().size(); ++i) {
                const auto* sourceStation = FindComponent<OpenSim::Station>(sourceModel, get_stations(i));
                OSC_ASSERT_ALWAYS(sourceStation && "could not find a station in the source model");

                // Find the input station and use it produce the warped station.
                const auto* resultStation = FindComponent<OpenSim::Station>(resultModel, get_stations(i));
                OSC_ASSERT_ALWAYS(resultStation && "could not find a station in the model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    resultModel,
                    sourceStation->get_location(),
                    resultStation->get_location(),
                    sourceStation->getParentFrame(),
                    resultStation->getParentFrame(),
                    *commonParams.sourceLandmarksFrame,
                    *commonParams.resultLandmarksFrame,
                    commonParams.tpsInputs,
                    commonParams.compensateForFrameChanges
                );

                auto* resultStationMut = FindComponentMut<OpenSim::Station>(resultModel, get_stations(i));
                OSC_ASSERT_ALWAYS(resultStationMut && "could not find a corresponding station in the result model");
                resultStationMut->set_location(warpedLocation);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };
}

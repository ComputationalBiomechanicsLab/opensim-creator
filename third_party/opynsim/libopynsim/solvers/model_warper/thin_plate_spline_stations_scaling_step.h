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

#include <format>
#include <string>

namespace OpenSim
{
    class Model;
}

namespace opyn
{
    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp to any
    // `OpenSim::Station`s it can find via the `stations` search string.
    class ThinPlateSplineStationsScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(
            ThinPlateSplineStationsScalingStep, ThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(
            stations,
            std::string,
            "Query paths (e.g. `/forceset/*`) that the engine should use to find stations in the source model that should be warped by this scaling step.");

    public:
        explicit ThinPlateSplineStationsScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Stations"}
        {
            setDescription(
                "Warps the locations of stations in the model using the Thin-Plate Spline (TPS) warping algorithm.");
            constructProperty_stations();
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

            // Ensure every entry in `stations` can be found in the source model.
            for (int i = 0; i < getProperty_stations().size(); ++i) {
                const auto* station =
                    find_component<OpenSim::Station>(source_model, get_stations(i));
                if (not station) {
                    messages.emplace_back(
                        ScalingStepValidationState::Error,
                        std::format(
                            "{}: Cannot a Station in 'stations' in the source model (or it isn't a Station).",
                            get_stations(i)));
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

            // Warp each station specified by the `stations` property.
            for (int i = 0; i < getProperty_stations().size(); ++i) {
                const auto* source_station =
                    find_component<OpenSim::Station>(source_model, get_stations(i));
                OSC_ASSERT_ALWAYS(source_station && "could not find a station in the source model");

                // Find the input station and use it produce the warped station.
                const auto* result_station =
                    find_component<OpenSim::Station>(result_model, get_stations(i));
                OSC_ASSERT_ALWAYS(result_station && "could not find a station in the model");

                const SimTK::Vec3 warped_location = scaling_cache.lookup_tps_warped_rigid_point(
                    source_model,
                    result_model,
                    source_station->get_location(),
                    result_station->get_location(),
                    source_station->getParentFrame(),
                    result_station->getParentFrame(),
                    *common_params.source_landmarks_frame,
                    *common_params.result_landmarks_frame,
                    common_params.tps_inputs,
                    common_params.compensate_for_frame_changes);

                auto* result_station_mut =
                    find_component_mut<OpenSim::Station>(result_model, get_stations(i));
                OSC_ASSERT_ALWAYS(
                    result_station_mut &&
                    "could not find a corresponding station in the result model");
                result_station_mut->set_location(warped_location);
            }
            initialize_model(result_model);
            initialize_state(result_model);
        }
    };
}

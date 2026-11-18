#pragma once

#include <libopynsim/solvers/model_warper/toggleable_thin_plate_spline_scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
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
    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp the centers
    // of mass of the given bodies (opensim-creator#1147).
    class ThinPlateSplineBodyCenterOfMassScalingStep final
        : public ToggleableThinPlateSplineScalingStep {

        OpenSim_DECLARE_CONCRETE_OBJECT(
            ThinPlateSplineBodyCenterOfMassScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(
            bodies,
            std::string,
            "Query paths (e.g. `/bodyset/*`) that the engine should use to find bodies in the source model that should be warped by this scaling step.");

    public:
        explicit ThinPlateSplineBodyCenterOfMassScalingStep() :
            ToggleableThinPlateSplineScalingStep{
                "Apply Thin-Plate Spline (TPS) to Body Centers of Mass"}
        {
            setDescription(
                "Warps the locations of body centers of mass in the model using the Thin-Plate Spline (TPS) warping algorithm.");
            constructProperty_bodies();
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

            // Ensure every entry in `bodies` can be found in the source model.
            for (int i = 0; i < getProperty_bodies().size(); ++i) {
                if (not find_component<OpenSim::Body>(source_model, get_bodies(i))) {
                    messages.emplace_back(
                        ScalingStepValidationState::Error,
                        std::format(
                            "{}: Cannot find a Body in 'bodies' in the source model (or it isn't a Body)",
                            get_bodies(i)));
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

            // Warp each specified center of mass, ensuring that the CoM is correctly
            // transformed into the TPS warp's coordinate system.
            for (int i = 0; i < getProperty_bodies().size(); ++i) {

                // Find the CoM in the source model.
                const auto* source_body =
                    find_component<OpenSim::Body>(source_model, get_bodies(i));
                OSC_ASSERT_ALWAYS(source_body && "could not find a body in the source model");

                // Find the CoM in the result model (i.e. the one that's midway through warping).
                const auto* result_body =
                    find_component<OpenSim::Body>(result_model, get_bodies(i));
                OSC_ASSERT_ALWAYS(result_body && "could not find a body in the result model");

                // Warp the CoM, while accounting for different frames etc. between the TPS
                // landmarks and the CoM.
                const SimTK::Vec3 warped_location = scaling_cache.lookup_tps_warped_rigid_point(
                    source_model,
                    result_model,
                    source_body->get_mass_center(),
                    result_body->get_mass_center(),
                    *source_body,
                    *result_body,
                    *common_params.source_landmarks_frame,
                    *common_params.result_landmarks_frame,
                    common_params.tps_inputs,
                    common_params.compensate_for_frame_changes);

                // Update the body with the new CoM
                auto* result_body_mut =
                    find_component_mut<OpenSim::Body>(result_model, get_bodies(i));
                OSC_ASSERT_ALWAYS(
                    result_body_mut && "could not find a corresponding body in the result model");
                result_body_mut->set_mass_center(warped_location);
            }
            initialize_model(result_model);
            initialize_state(result_model);
        }
    };
}

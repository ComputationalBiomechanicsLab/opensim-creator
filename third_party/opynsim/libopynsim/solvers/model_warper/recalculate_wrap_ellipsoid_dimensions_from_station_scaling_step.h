#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/Wrap/WrapEllipsoid.h>
#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/Transform.h>

#include <format>
#include <string>
#include <vector>

namespace OpenSim
{
    class Model;
}

namespace opyn
{
    class RecalculateWrapEllipsoidDimensionsFromStationScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(
            RecalculateWrapEllipsoidDimensionsFromStationScalingStep, ScalingStep)

    public:
        OpenSim_DECLARE_PROPERTY(
            station_path,
            std::string,
            "Absolute path (e.g. `/componentset/some_station`) to a `Station` component in the model");
        OpenSim_DECLARE_PROPERTY(
            wrap_ellipsoid_path,
            std::string,
            "Absolute path (e.g. `/bodyset/body/wrap_ellipsoid`) to a `WrapEllipsoid` component in the model");
        OpenSim_DECLARE_PROPERTY(
            dimensions_scale_factors,
            SimTK::Vec3,
            "A scaling factor that's multiplied by the calculated distance when calculating each dimension of the `WrapEllipsoid`");

        explicit RecalculateWrapEllipsoidDimensionsFromStationScalingStep() :
            ScalingStep{"Recalculate WrapEllipsoid `dimensions` from distance to Station"}
        {
            setDescription(
                "Recalculates the 'dimensions' of a `WrapEllipsoid` component, located at `wrap_ellipsoid_path`, as the distance between the `Station`, located at `station_path`, to the `WrapEllipsoid`'s origin, scaled by `dimensions_scale_factors`.");
            constructProperty_station_path("");
            constructProperty_wrap_ellipsoid_path("");
            constructProperty_dimensions_scale_factors(SimTK::Vec3{1.0});
        }

    private:
        std::vector<ScalingStepValidationMessage> impl_validate(
            ScalingCache&, const ScalingParameters&, const OpenSim::Model& model) const override
        {
            std::vector<ScalingStepValidationMessage> messages;

            // Ensure `station_path` exists in the model (and is a `Station`)
            const auto* station = find_component<OpenSim::Station>(model, get_station_path());
            if (not station) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format(
                        "{}: Cannot find `station_path` in the source model (or it isn't a Station).",
                        get_station_path()));
            }

            // Ensure `wrap_ellipsoid_path` exists in the model (and is a `WrapEllipsoid`)
            const auto* wrap_ellipsoid =
                find_component<OpenSim::WrapEllipsoid>(model, get_wrap_ellipsoid_path());
            if (not wrap_ellipsoid) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format(
                        "{}: Cannot find 'wrap_ellipsoid_path' in the source model (or it isn't a `WrapEllipsoid`).",
                        get_wrap_ellipsoid_path()));
            }

            return messages;
        }

        void impl_apply_scaling_step(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&,
            OpenSim::Model& result_model) const override
        {
            const auto* station =
                find_component<OpenSim::Station>(result_model, get_station_path());
            OSC_ASSERT_ALWAYS(station && "could not find a station in the model");
            auto* wrap_ellipsoid =
                find_component_mut<OpenSim::WrapEllipsoid>(result_model, get_wrap_ellipsoid_path());
            OSC_ASSERT_ALWAYS(wrap_ellipsoid && "could not find a `WrapEllipsoid` in the model");

            // Put the station into the `WrapEllipsoid`'s reference frame.
            const SimTK::Transform ellipsoid2ellipsoid_frame = wrap_ellipsoid->getTransform();
            const SimTK::Transform ellipsoid_frame2ground =
                wrap_ellipsoid->getFrame().getTransformInGround(result_model.getWorkingState());
            const SimTK::Transform ground2ellipsoid =
                (ellipsoid_frame2ground * ellipsoid2ellipsoid_frame).invert();
            const SimTK::Vec3 station_in_ellipsoid =
                ground2ellipsoid * station->getLocationInGround(result_model.getWorkingState());

            const auto euclidian_distance_to_origin = station_in_ellipsoid.norm();

            // The new dimensions of the `WrapEllipsoid` is the multiplication of the distance to
            // the origin and the scale factors.
            const SimTK::Vec3 new_ellipsoid_dimensions =
                euclidian_distance_to_origin * get_dimensions_scale_factors();

            // Update the `WrapEllipsoid` accordingly and reinitialize the model.
            wrap_ellipsoid->set_dimensions(new_ellipsoid_dimensions);
            initialize_model(result_model);
            initialize_state(result_model);
        }
    };
}

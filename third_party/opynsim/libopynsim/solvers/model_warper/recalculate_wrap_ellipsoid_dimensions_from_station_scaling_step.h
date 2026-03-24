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

#include <sstream>
#include <string>
#include <vector>

namespace OpenSim { class Model; }

namespace opyn
{
    class RecalculateWrapEllipsoidDimensionsFromStationScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(RecalculateWrapEllipsoidDimensionsFromStationScalingStep, ScalingStep)
    public:
        OpenSim_DECLARE_PROPERTY(station_path, std::string, "Absolute path (e.g. `/componentset/some_station`) to a `Station` component in the model");
        OpenSim_DECLARE_PROPERTY(wrap_ellipsoid_path, std::string, "Absolute path (e.g. `/bodyset/body/wrap_ellipsoid`) to a `WrapEllipsoid` component in the model");
        OpenSim_DECLARE_PROPERTY(dimensions_scale_factors, SimTK::Vec3, "A scaling factor that's multiplied by the calculated distance when calculating each dimension of the `WrapEllipsoid`");

        explicit RecalculateWrapEllipsoidDimensionsFromStationScalingStep() :
            ScalingStep{"Recalculate WrapEllipsoid `dimensions` from distance to Station"}
        {
            setDescription("Recalculates the 'dimensions' of a `WrapEllipsoid` component, located at `wrap_ellipsoid_path`, as the distance between the `Station`, located at `station_path`, to the `WrapEllipsoid`'s origin, scaled by `dimensions_scale_factors`.");
            constructProperty_station_path("");
            constructProperty_wrap_ellipsoid_path("");
            constructProperty_dimensions_scale_factors(SimTK::Vec3{1.0});
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model& model) const final
        {
            std::vector<ScalingStepValidationMessage> messages;

            // Ensure `station_path` exists in the model (and is a `Station`)
            const auto* station = FindComponent<OpenSim::Station>(model, get_station_path());
            if (not station) {
                std::stringstream msg;
                msg << get_station_path() << ": Cannot find `station_path` in the source model (or it isn't a Station).";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            // Ensure `wrap_ellipsoid_path` exists in the model (and is a `WrapEllipsoid`)
            const auto* wrapEllipsoid = FindComponent<OpenSim::WrapEllipsoid>(model, get_wrap_ellipsoid_path());
            if (not wrapEllipsoid) {
                std::stringstream msg;
                msg << get_wrap_ellipsoid_path() << ": Cannot find 'wrap_ellipsoid_path' in the source model (or it isn't a `WrapEllipsoid`).";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            const auto* station = FindComponent<OpenSim::Station>(resultModel, get_station_path());
            OSC_ASSERT_ALWAYS(station && "could not find a station in the model");
            auto* wrapEllipsoid = FindComponentMut<OpenSim::WrapEllipsoid>(resultModel, get_wrap_ellipsoid_path());
            OSC_ASSERT_ALWAYS(wrapEllipsoid && "could not find a `WrapEllipsoid` in the model");

            // Put the station into the `WrapEllipsoid`'s reference frame.
            const SimTK::Transform ellipsoid2ellipsoidFrame = wrapEllipsoid->getTransform();
            const SimTK::Transform ellipsoidFrame2ground = wrapEllipsoid->getFrame().getTransformInGround(resultModel.getWorkingState());
            const SimTK::Transform ground2ellipsoid = (ellipsoidFrame2ground * ellipsoid2ellipsoidFrame).invert();
            const SimTK::Vec3 stationInEllipsoid = ground2ellipsoid * station->getLocationInGround(resultModel.getWorkingState());

            const auto euclidianDistanceToOrigin = stationInEllipsoid.norm();

            // The new dimensions of the `WrapEllipsoid` is the multiplication of the distance to
            // the origin and the scale factors.
            const SimTK::Vec3 newEllipsoidDimensions = euclidianDistanceToOrigin * get_dimensions_scale_factors();

            // Update the `WrapEllipsoid` accordingly and reinitialize the model.
            wrapEllipsoid->set_dimensions(newEllipsoidDimensions);
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };
}

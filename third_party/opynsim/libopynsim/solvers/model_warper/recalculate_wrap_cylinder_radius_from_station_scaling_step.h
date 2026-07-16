#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/Wrap/WrapCylinder.h>
#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/Transform.h>

#include <format>
#include <string>
#include <vector>

namespace OpenSim { class Model; }

namespace opyn
{
    class RecalculateWrapCylinderRadiusFromStationScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(RecalculateWrapCylinderRadiusFromStationScalingStep, ScalingStep)
    public:
        OpenSim_DECLARE_PROPERTY(station_path, std::string, "Absolute path (e.g. `/componentset/some_station`) to a `Station` component in the model");
        OpenSim_DECLARE_PROPERTY(wrap_cylinder_path, std::string, "Absolute path (e.g. `/bodyset/body/wrap_cylinder_2`) to a `WrapCylinder` component in the model");

        explicit RecalculateWrapCylinderRadiusFromStationScalingStep() :
            ScalingStep{"Recalculate WrapCylinder `radius` from Station Projection onto its Midline"}
        {
            setDescription("Recalculates the 'radius' of a `WrapCylinder` component, located at `wrap_cylinder_path`, as the distance between the `Station`, located at `station_path`, and the cylinder's (infinitely long) midline.");
            constructProperty_station_path("");
            constructProperty_wrap_cylinder_path("");
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
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format("{}: Cannot find `station_path` in the source model (or it isn't a Station).", get_station_path())
                );
            }

            // Ensure `wrap_cylinder_path` exists in the model (and is a `WrapCylinder`)
            const auto* wrapCylinder = FindComponent<OpenSim::WrapCylinder>(model, get_wrap_cylinder_path());
            if (not wrapCylinder) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format("{}: Cannot find 'wrap_cylinder_path' in the source model (or it isn't a `WrapCylinder`).", get_wrap_cylinder_path())
                );
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
            auto* wrapCylinder = FindComponentMut<OpenSim::WrapCylinder>(resultModel, get_wrap_cylinder_path());
            OSC_ASSERT_ALWAYS(wrapCylinder && "could not find a wrap cylinder in the model");

            // Put the station into the cylinder's reference frame
            const SimTK::Transform cylinder2cylinderFrame = wrapCylinder->getTransform();
            const SimTK::Transform cylinderFrame2ground = wrapCylinder->getFrame().getTransformInGround(resultModel.getWorkingState());
            const SimTK::Transform ground2cylinder = (cylinderFrame2ground * cylinder2cylinderFrame).invert();
            const SimTK::Vec3 pCylinder = ground2cylinder * station->getLocationInGround(resultModel.getWorkingState());

            // In the cylinder's frame, Z (from origin) is the centerline of the cylinder, so the easiest way to
            // figure out the new radius is just to compute the XY norm from the origin and ignore Z.
            const auto newRadius = SimTK::Vec2{pCylinder[0], pCylinder[1]}.norm();

            // Update accordingly
            wrapCylinder->set_radius(newRadius);
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };
}

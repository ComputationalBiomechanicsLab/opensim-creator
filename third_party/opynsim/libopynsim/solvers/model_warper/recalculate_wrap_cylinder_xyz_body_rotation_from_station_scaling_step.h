#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/Wrap/WrapCylinder.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/Transform.h>
#include <SimTKcommon/internal/UnitVec.h>
#include <SimTKcommon/SmallMatrix.h>

#include <cmath>
#include <format>
#include <string>
#include <vector>

namespace OpenSim
{
    class Model;
}

namespace opyn
{
    class RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(
            RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep, ScalingStep)

    public:
        OpenSim_DECLARE_PROPERTY(
            station_path,
            std::string,
            "Absolute path (e.g. `/componentset/some_station`) to a `Station` component in the model");
        OpenSim_DECLARE_PROPERTY(
            wrap_cylinder_path,
            std::string,
            "Absolute path (e.g. `/bodyset/body/wrap_cylinder_2`) to a `WrapCylinder` component in the model");

        explicit RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep() :
            ScalingStep{
                "Recalculate WrapCylinder 'xyz_body_rotation' from Station Placed Along Its Midline"}
        {
            setDescription(
                "Recalculates the 'xyz_body_rotation' of a `WrapCylinder` component, located at `wrap_cylinder_path` such that the cylinder's +Z direction (midline) points toward the Station component located at `station_path`");
            constructProperty_station_path("");
            constructProperty_wrap_cylinder_path("");
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

            // Ensure `wrap_cylinder_path` exists in the model (and is a `WrapCylinder`)
            const auto* wrap_cylinder =
                find_component<OpenSim::WrapCylinder>(model, get_wrap_cylinder_path());
            if (not wrap_cylinder) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format(
                        "{}: Cannot find 'wrap_cylinder_path' in the source model (or it isn't a `WrapCylinder`).",
                        get_wrap_cylinder_path()));
            }

            return messages;
        }

        void impl_apply_scaling_step(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&,
            OpenSim::Model& result_model) const override
        {
            using std::abs;
            using std::acos;

            const auto* station =
                find_component<OpenSim::Station>(result_model, get_station_path());
            OSC_ASSERT_ALWAYS(station && "could not find a station in the model");
            auto* wrap_cylinder =
                find_component_mut<OpenSim::WrapCylinder>(result_model, get_wrap_cylinder_path());
            OSC_ASSERT_ALWAYS(wrap_cylinder && "could not find a wrap cylinder in the model");

            // Re-express the station in the cylinder's reference frame
            const SimTK::Transform cylinder2cylinder_frame = wrap_cylinder->getTransform();
            const SimTK::Transform cylinder_frame2ground =
                wrap_cylinder->getFrame().getTransformInGround(result_model.getWorkingState());
            const SimTK::Transform ground2cylinder =
                (cylinder_frame2ground * cylinder2cylinder_frame).invert();
            const SimTK::Vec3 station_pos_in_cylinder =
                ground2cylinder * station->getLocationInGround(result_model.getWorkingState());

            // Calculate the source/target cylinder midline directions.
            const SimTK::UnitVec3 cylinder_direction_in_cylinder{
                SimTK::CoordinateAxis::ZCoordinateAxis{}};
            const SimTK::UnitVec3 station_direction_in_cylinder{station_pos_in_cylinder};

            // Edge-case: If the station is pointing along the Z axis, no reorientation is necessary
            const auto cos_rotation_angle =
                SimTK::dot(cylinder_direction_in_cylinder, station_direction_in_cylinder);
            if (abs(cos_rotation_angle) >= 1.0 - SimTK::Eps) {
                return;  // Nothing to do
            }

            // Else: If the station isn't pointing along the Z axis (usual case), calculate a
            // rotation that
            //       rotates the cylinder's coordinate space so that +Z points toward toward the
            //       station.
            const auto rotation_angle = acos(cos_rotation_angle);
            const SimTK::UnitVec3 rotation_axis{
                SimTK::cross(cylinder_direction_in_cylinder, station_direction_in_cylinder)};
            const SimTK::Rotation rotation{rotation_angle, rotation_axis};

            // Compose the additional rotation with the original one to calculate the necessary
            // overall rotation
            const SimTK::Rotation new_rotation = cylinder2cylinder_frame.R() * rotation;

            // Write the new rotation to the `WrapCylinder`'s `xyz_body_rotation` property as Euler
            // angles
            wrap_cylinder->set_xyz_body_rotation(new_rotation.convertRotationToBodyFixedXYZ());
            initialize_model(result_model);
            initialize_state(result_model);
        }
    };
}

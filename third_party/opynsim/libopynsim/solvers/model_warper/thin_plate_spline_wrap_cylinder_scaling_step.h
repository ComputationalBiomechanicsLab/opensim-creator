#pragma once

#include <libopynsim/solvers/model_warper/toggleable_thin_plate_spline_scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Wrap/WrapCylinder.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/UnitVec.h>
#include <SimTKcommon/SmallMatrix.h>

#include <format>
#include <string>
#include <vector>

namespace opyn
{
        // A `ThinPlateSpline` scaling step that tries to scale the origin, orientation, radius,
    // length, and quadrant of the given `WrapCylinder`s.
    class ThinPlateSplineWrapCylinderScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineWrapCylinderScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(wrap_cylinders, std::string, "Absolute paths (e.g. `/bodyset/body/wrap_cylinder_2`) to `WrapCylinder` components in the model");
        OpenSim_DECLARE_PROPERTY(midline_projection_distance, double, "The distance, in meters, that the `WrapCylinder`s' midlines should be projected from each of their origin points before putting them through the TPS algorithm. This is used to figure out the new orientation of the `WrapCylinder`s.");
        OpenSim_DECLARE_PROPERTY(surface_projection_theta, double, "An angle, in radians, of a vector that originates in each `WrapCylinder`'s origin, spins around their Z axis with an angle of theta from the X axis, and has a length of each `WrapCylinder`'s `radius`, producing a surface point. Each surface point is fed through the TPS algorithm and used to recalculate each `WrapCylinder`'s `radius` and `quadrant`");
    public:
        explicit ThinPlateSplineWrapCylinderScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to WrapCylinder"}
        {
            setDescription(R"(
Uses the Thin-Plate Spline (TPS) warping algorithm to scale `WrapCylinder`s in the model:

  - Warps the `translation` of each `WrapCylinder` by warping the source `translation` with
    the TPS algorithm to produce a new `translation`.
  - Warps the `xyz_body_rotation` of each `WrapCylinder` by projecting a point from its
    source origin (`translation`) `midline_projection_distance` along the cylinder's
    midline (+Z), warping it with the TPS algorithm, and then calculating a new
    `xyz_body_orientation` that orients the `WrapCylinder` such that the vector between the
    new origin (the new `translation`) and the projected point becomes the new
    `WrapCylinder` midline. To prevent the cylinder from spinning along this vector, the
    along-axis rotation is constrained by the location of a point produced by
    `surface_projection_theta`.
  - Warps the `radius` of each `WrapCylinder` by calculating a point on each of their surfaces
    specified `surface_projection_theta`. The points are then warped with the TPS algorithm to
    produce (presumed to be) new surface points. The `radius` of each `WrapCylinder` is equal
    to the distance between their respective midlines and projected surface points.
  - Does not warp `quadrant`. It is assumed that constraining the calculation of `xyz_body_rotation`
    will cause the warped cylinder to have a like-for-like orientation with respect to whatever's
    being wrapped.
  - Does not warp `length`. There is (currently) no easy way to do this, because the ends of
    `WrapCylinder`s tend to be far away from where the TPS warp is defined, which makes it hard
    to warp+project the endcaps. It is recommended that `WrapCylinder`s are long enough to ensure
    they are not sensitive to model scaling (very few models require muscles to fall of the end
    of the cylinder).
)");
            constructProperty_wrap_cylinders();
            constructProperty_midline_projection_distance(0.001);  // 1 mm
            constructProperty_surface_projection_theta(0.0);
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure every entry in `wrap_cylinders` can be found in the source model.
            for (int i = 0; i < getProperty_wrap_cylinders().size(); ++i) {
                const auto* offsetFrame = FindComponent<OpenSim::WrapCylinder>(sourceModel, get_wrap_cylinders(i));
                if (not offsetFrame) {
                    messages.emplace_back(
                        ScalingStepValidationState::Error,
                        std::format("{}: Cannot find a `WrapCylinder` in 'wrap_cylinders' in the source model (or it isn't a `WrapCylinder`).", get_wrap_cylinders(i))
                    );
                }
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& cache,
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel,
            OpenSim::Model& resultModel) const final
        {
            // Lookup/validate warping inputs.
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel, resultModel);

            // Precalculate the surface point direction vector as "rotate a unit vector pointing
            // along X around the Z axis by theta amount" (see property documentation).
            const SimTK::Rotation rotateCylinderXToSurfacePointDirection{get_surface_projection_theta(), SimTK::ZAxis};
            const SimTK::UnitVec3 surfacePointDirectionInCylinderSpace{rotateCylinderXToSurfacePointDirection * SimTK::Vec3{1.0, 0.0, 0.0}};

            // Precalculate the midline projection point in the cylinder's local (not parent) space.
            const SimTK::Vec3 midlinePointInCylinderSpace{0.0, 0.0, get_midline_projection_distance()};

            // Warp each `WrapCylinder` specified by the `wrap_cylinders` property.
            for (int i = 0; i < getProperty_wrap_cylinders().size(); ++i) {
                const auto* sourceWrapCylinder = FindComponent<OpenSim::WrapCylinder>(sourceModel, get_wrap_cylinders(i));
                OSC_ASSERT_ALWAYS(sourceWrapCylinder && "could not find a `WrapCylinder` in the source model");

                // Find the `i`th `WrapCylinder` in the model.
                auto* resultWrapCylinder = FindComponentMut<OpenSim::WrapCylinder>(resultModel, get_wrap_cylinders(i));
                OSC_ASSERT_ALWAYS(resultWrapCylinder && "could not find a `WrapCylinder` in the model");

                // Calculate the `WrapCylinder`'s new `translation` by warping the origin.
                const SimTK::Vec3 newOriginPointInParent = cache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    resultModel,
                    sourceWrapCylinder->get_translation(),
                    resultWrapCylinder->get_translation(),
                    sourceWrapCylinder->getFrame(),
                    resultWrapCylinder->getFrame(),
                    *commonParams.sourceLandmarksFrame,
                    *commonParams.resultLandmarksFrame,
                    commonParams.tpsInputs,
                    commonParams.compensateForFrameChanges
                );

                // Calculate the `WrapCylinder`'s new projected midline point by warping it.
                const SimTK::Vec3 sourceMidlinePointInParent = sourceWrapCylinder->getTransform() * midlinePointInCylinderSpace;
                const SimTK::Vec3 resultMidlinePointInParent = resultWrapCylinder->getTransform() * midlinePointInCylinderSpace;
                const SimTK::Vec3 newMidlinePointInParent = cache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    resultModel,
                    sourceMidlinePointInParent,
                    resultMidlinePointInParent,
                    sourceWrapCylinder->getFrame(),
                    resultWrapCylinder->getFrame(),
                    *commonParams.sourceLandmarksFrame,
                    *commonParams.resultLandmarksFrame,
                    commonParams.tpsInputs,
                    commonParams.compensateForFrameChanges
                );

                // Calculate the source surface point by projecting the direction onto the `WrapCylinder`'s surface.
                const SimTK::Vec3 sourceSurfacePointInCylinderSpace = sourceWrapCylinder->get_radius() * surfacePointDirectionInCylinderSpace;
                const SimTK::Vec3 resultSurfacePointInCylinderSpace = resultWrapCylinder->get_radius() * surfacePointDirectionInCylinderSpace;
                const SimTK::Vec3 sourceSurfacePointInParent = sourceWrapCylinder->getTransform() * sourceSurfacePointInCylinderSpace;
                const SimTK::Vec3 resultSurfacePointInParent = resultWrapCylinder->getTransform() * resultSurfacePointInCylinderSpace;
                const SimTK::Vec3 newSurfacePointInParent = cache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    resultModel,
                    sourceSurfacePointInParent,
                    resultSurfacePointInParent,
                    sourceWrapCylinder->getFrame(),
                    resultWrapCylinder->getFrame(),
                    *commonParams.sourceLandmarksFrame,
                    *commonParams.resultLandmarksFrame,
                    commonParams.tpsInputs,
                    commonParams.compensateForFrameChanges
                );

                // The `WrapCylinder`'s new Z axis within the parent frame is a unit vector that
                // points from its new origin to the projected midline point.
                const SimTK::UnitVec3 newZAxisDirectionInParent{newMidlinePointInParent - newOriginPointInParent};

                // We can now use the warped surface point to figure out what the orientation _around_
                // Z should be (i.e. to constrain it, rather than letting the orientation spin along
                // the Z axis).
                //
                // Due to the non-linearity of TPS, the warped surface point may require
                // reorthogonalization with respect to the `WrapCylinder`'s new midline. Here, we use
                // vector rejection (the opposite of vector projection) to compute an orthogonal vector
                // that we can un-spin by `surface_projection_theta` in order to figure out exactly where
                // the X axis should point.
                const SimTK::Vec3 newSurfacePointVectorInParent{newSurfacePointInParent - newOriginPointInParent};
                const SimTK::UnitVec3 newSurfacePointDirectionInParent{newSurfacePointVectorInParent};
                OSC_ASSERT((SimTK::dot(newZAxisDirectionInParent, newSurfacePointDirectionInParent) < 1.0 - SimTK::Eps) && "cylinder surface point somehow points along Z axis - warping is too strong?");
                const SimTK::Vec3 newSurfacePointProjectionAlongZVector = newZAxisDirectionInParent * SimTK::dot(newZAxisDirectionInParent, newSurfacePointDirectionInParent);
                const SimTK::Vec3 newSurfacePointRejectionVector = newSurfacePointVectorInParent - newSurfacePointProjectionAlongZVector;
                const SimTK::UnitVec3 newSurfacePointRejectionDirection{newSurfacePointRejectionVector};

                // The new surface point direction is assumed to be `surface_projection_theta` rotated along
                // the new Z axis from where the X axis should be (that's how it was initially generated), so
                // un-rotate it to figure out where the new X axis should be.
                const SimTK::UnitVec3 newXAxisDirectionInParent{rotateCylinderXToSurfacePointDirection.invert() * newSurfacePointRejectionDirection};

                // With two vectors pointing along known axes, we can compute a new cylinder rotation
                const SimTK::Rotation newCylinderRotation{newZAxisDirectionInParent, SimTK::ZAxis, newXAxisDirectionInParent, SimTK::XAxis};

                // The new radius is the length of the rejection vector: i.e. the length of a vector
                // orthogonal to Z (rejection) that points toward the surface point. Another way of
                // thinking about this is that it's the shortest distance between the midline and
                // the point
                const double newRadius = newSurfacePointVectorInParent.norm();

                // (finally) Update the cylinder
                resultWrapCylinder->set_translation(newOriginPointInParent);
                resultWrapCylinder->set_xyz_body_rotation(newCylinderRotation.convertRotationToBodyFixedXYZ());
                resultWrapCylinder->set_radius(newRadius);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };
}

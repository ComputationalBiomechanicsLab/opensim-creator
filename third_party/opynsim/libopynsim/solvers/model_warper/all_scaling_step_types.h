#pragma once

#include <libopynsim/solvers/model_warper/manually_scale_body_segments_scaling_step.h>
#include <libopynsim/solvers/model_warper/model_mass_to_subject_mass_scaling_step.h>
#include <libopynsim/solvers/model_warper/recalculate_wrap_cylinder_radius_from_station_scaling_step.h>
#include <libopynsim/solvers/model_warper/recalculate_wrap_cylinder_xyz_body_rotation_from_station_scaling_step.h>
#include <libopynsim/solvers/model_warper/recalculate_wrap_ellipsoid_dimensions_from_station_scaling_step.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_body_center_of_mass_scaling_step.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_mesh_substitution_scaling_step.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_meshes_scaling_step.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_offset_frame_translation_scaling_step.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_path_points_scaling_step.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_stations_scaling_step.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_wrap_cylinder_scaling_step.h>

#include <liboscar/utilities/typelist.h>

#include <array>
#include <memory>

namespace opyn
{
    // Compile-time `Typelist` containing all `ScalingStep`s that this UI handles.
    using AllScalingStepTypes = osc::Typelist<
        ThinPlateSplineMeshesScalingStep,
        ThinPlateSplineStationsScalingStep,
        ThinPlateSplinePathPointsScalingStep,
        ThinPlateSplineBodyCenterOfMassScalingStep,
        ThinPlateSplineOffsetFrameTranslationScalingStep,
        ThinPlateSplineMeshSubstitutionScalingStep,
        ThinPlateSplineWrapCylinderScalingStep,
        ModelMassToSubjectMassScalingStep,
        ManuallyScaleBodySegmentsScalingStep,
        RecalculateWrapCylinderRadiusFromStationScalingStep,
        RecalculateWrapEllipsoidDimensionsFromStationScalingStep,
        RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep
    >;

    // Returns a list of `ScalingStep` prototypes, so that downstream code is able to present
    // them as available options etc.
    const auto& getScalingStepPrototypes()
    {
        static const auto s_ScalingStepPrototypes = []<typename... TScalingStep>(osc::Typelist<TScalingStep...>)
        {
            return std::to_array<std::unique_ptr<ScalingStep>>({
                std::make_unique<TScalingStep>()...
            });
        }(AllScalingStepTypes{});

        return s_ScalingStepPrototypes;
    }
}

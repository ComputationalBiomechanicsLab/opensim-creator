#pragma once

#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_scaling_step.h>

#include <string_view>

namespace OpenSim { class Model; }

namespace opyn
{
    // An abstract base class for a `ThinPlateSplineScalingStep` that has user-editable
    // toggles for parts of the TPS algorithm.
    class ToggleableThinPlateSplineScalingStep : public ThinPlateSplineScalingStep {
        OpenSim_DECLARE_ABSTRACT_OBJECT(ToggleableThinPlateSplineScalingStep, ThinPlateSplineScalingStep)
    public:
        OpenSim_DECLARE_PROPERTY(apply_affine_translation, bool, "Enable/disable applying the affine translational part of the TPS warp to the output. This can be useful/necessary if/when it's handled by some other part of the model (e.g. an offset frame, joint frame).");
        OpenSim_DECLARE_PROPERTY(apply_affine_scale,       bool, "Enable/disable applying the affine scaling part of the TPS warp to the output. This can be useful/necessary for visually inspecting the difference between the non-affine scaling components and the affine parts.");
        OpenSim_DECLARE_PROPERTY(apply_affine_rotation,    bool, "Enable/disable applying the application of the affine rotational part of the TPS warp to the output. This can be useful/necessary if/when it's handled by some other part of the model (e.g. an offset frame, joint frame).");
        OpenSim_DECLARE_PROPERTY(apply_non_affine_warp,    bool, "Enable/disable applying the non-affine (i.e. the 'warp-ey part') of the TPS warp to the output.");

    protected:
        explicit ToggleableThinPlateSplineScalingStep(std::string_view label) :
            ThinPlateSplineScalingStep{label}
        {
            constructProperty_apply_affine_translation(false);
            constructProperty_apply_affine_scale(true);
            constructProperty_apply_affine_rotation(false);
            constructProperty_apply_non_affine_warp(true);
        }

        CommonParameters calcTPSScalingStepCommonParams(
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel,
            const OpenSim::Model& resultModel) const
        {
            auto rv = ThinPlateSplineScalingStep::calcTPSScalingStepCommonParams(parameters, sourceModel, resultModel);
            rv.tpsInputs.applyAffineTranslation = get_apply_affine_translation();
            rv.tpsInputs.applyAffineScale = get_apply_affine_scale();
            rv.tpsInputs.applyAffineRotation = get_apply_affine_rotation();
            rv.tpsInputs.applyNonAffineWarp = get_apply_non_affine_warp();
            return rv;
        }
    };
}

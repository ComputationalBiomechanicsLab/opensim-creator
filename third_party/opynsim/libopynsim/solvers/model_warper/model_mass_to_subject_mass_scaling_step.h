#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>

#include <liboscar/utilities/assertions.h>

#include <functional>
#include <optional>
#include <vector>

namespace OpenSim { class Model; }

namespace opyn
{
    // A `ScalingStep` that scales the masses of bodies in the model.
    class ModelMassToSubjectMassScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ModelMassToSubjectMassScalingStep, ScalingStep)
    public:
        explicit ModelMassToSubjectMassScalingStep() :
            ScalingStep{"Scale Model Mass to Subject Mass"}
        {
            setDescription("Scales the masses of bodies in the model to match the subject mass, while preserving the overall mass disribution of each body in the model.");
        }

    private:
        std::vector<ScalingStepValidationMessage> impl_validate(
            ScalingCache&,
            const ScalingParameters& parameters,
            const OpenSim::Model& model) const final
        {
            std::vector<ScalingStepValidationMessage> messages;

            if (parameters.lookup<double>("subject_mass").value_or(0.0) <= 0.0) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    "The subject_mass scaling parameter must be greater than zero"
                );
            }

            if (model.getTotalMass(model.getWorkingState()) <= 0.0) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    "Cannot scale the model's mass to the subject's mass because the model itself has a mass of zero"
                );
            }

            return messages;
        }

        void impl_for_each_scaling_parameter_declaration(const std::function<void(const ScalingParameterDeclaration&)>& callback) const final
        {
            callback(ScalingParameterDeclaration{"blending_factor", 1.0});
            callback(ScalingParameterDeclaration{"subject_mass", 75.0});
        }

        void impl_apply_scaling_step(
            ScalingCache&,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            const auto subjectMass = parameters.lookup<double>("subject_mass").value_or(0.0);
            OSC_ASSERT_ALWAYS(subjectMass > 0.0 && "Subject mass must be greater than zero");

            const std::optional<double> blendingFactor = parameters.lookup<double>("blending_factor");
            OSC_ASSERT_ALWAYS(blendingFactor && "blending_factor was not set by the warping engine");

            const auto effectiveTargetMass = osc::lerp(
                resultModel.getTotalMass(resultModel.getWorkingState()),
                subjectMass,
                *blendingFactor
            );

            scale_model_mass_preserve_mass_distribution(resultModel, resultModel.getWorkingState(), effectiveTargetMass);
            initialize_model(resultModel);
            initialize_state(resultModel);
        }
    };
}

#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/maths/common_functions.h>
#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Common/ScaleSet.h>
#include <SimTKcommon/SmallMatrix.h>

#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace opyn
{
    // A `ScalingStep` that scales body segments by the chosen (potentially, non-uniform) scale factors
    class ManuallyScaleBodySegmentsScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ManuallyScaleBodySegmentsScalingStep, ScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(bodies,          std::string, "Absolute paths (e.g. `/bodyset/femur`) to `Body` components in the model");
        OpenSim_DECLARE_PROPERTY(     scale_factors,   SimTK::Vec3, "The manual scale factors that should be applied to the specified bodies");
        OpenSim_DECLARE_PROPERTY(     preserve_masses, bool,        "If enabled, the masses of the bodies will not be scaled (inertial properties will still be scaled)");
    public:
        explicit ManuallyScaleBodySegmentsScalingStep() :
            ScalingStep{"Manually Scale Body Segments"}
        {
            setDescription("Applies a manually-specified scaling factor to the given body segments in the model.");
            constructProperty_bodies();
            constructProperty_scale_factors(SimTK::Vec3{1.0});
            constructProperty_preserve_masses(false);
        }

    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache&,
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel) const final
        {
            std::vector<ScalingStepValidationMessage> messages;

            // Ensure every entry in `wrap_cylinders` can be found in the source model.
            for (int i = 0; i < getProperty_bodies().size(); ++i) {
                const auto* body = FindComponent<OpenSim::Body>(sourceModel, get_bodies(i));
                if (not body) {
                    messages.emplace_back(
                        ScalingStepValidationState::Error,
                        std::format("{}: Cannot find a `Body` in 'bodies' in the source model (or it isn't a `Body`).", get_bodies(i))
                    );
                }
            }

            const std::optional<double> blendingFactor = parameters.lookup<double>("blending_factor");
            OSC_ASSERT_ALWAYS(blendingFactor && "blending_factor was not set by the warping engine");

            return messages;
        }

        void implForEachScalingParameterDeclaration(const std::function<void(const ScalingParameterDeclaration&)>& callback) const final
        {
            callback(ScalingParameterDeclaration{"blending_factor", 1.0});
        }

        void implApplyScalingStep(
            ScalingCache&,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& model) const final
        {
            const std::optional<double> blendingFactor = parameters.lookup<double>("blending_factor");
            OSC_ASSERT_ALWAYS(blendingFactor && "blending_factor was not set by the warping engine");

            const SimTK::Vec3 scaleFactors = {
                osc::lerp(1.0, get_scale_factors()[0], *blendingFactor),
                osc::lerp(1.0, get_scale_factors()[1], *blendingFactor),
                osc::lerp(1.0, get_scale_factors()[2], *blendingFactor),
            };

            OpenSim::ScaleSet scaleSet;
            for (int i = 0; i < getProperty_bodies().size(); ++i) {
                const auto* body = FindComponent<OpenSim::Body>(model, get_bodies(i));
                if (not body) {
                    auto msg = std::format("{}: Cannot find a `Body` in 'bodies' in the source model (or it isn't a `Body`).", get_bodies(i));
                    throw std::runtime_error{std::move(msg)};
                }

                auto scale = std::make_unique<OpenSim::Scale>();
                scale->setSegmentName(body->getName());
                scale->setScaleFactors(scaleFactors);
                scale->setApply(true);
                scaleSet.adoptAndAppend(scale.release());
            }
            model.scale(model.updWorkingState(), scaleSet, get_preserve_masses());
            InitializeModel(model);
            InitializeState(model);
        }
    };
}

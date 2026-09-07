#include "model_warper.h"

#include <libopynsim/documents/custom_components/in_memory_mesh.h>
#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/solvers/model_warper/model_warper_v3_document.h>
#include <libopynsim/solvers/model_warper/scaling_document_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/model.h>
#include <libopynsim/model_specification.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>

#include <liboscar/utilities/copy_on_upd_shared_value.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <cstddef>
#include <filesystem>
#include <format>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace opyn;

namespace
{
    /// Utility adaptor that adapts OPynSim's `Model`/`ModelState` to
    /// the `ModelStatePair` API.
    class OPynSimModelStatePair : public ModelStatePair {
    public:
        explicit OPynSimModelStatePair(Model model, ModelState model_state) :
            model_{std::move(model)},
            model_state_{std::move(model_state)}
        {}
    private:
        const OpenSim::Model& implGetModel() const override { return model_.open_sim_model(); }
        const SimTK::State& implGetState()   const override { return model_state_.simbody_state(); }

        Model model_;
        ModelState model_state_;
    };
}

class opyn::ModelWarper::Impl {
public:
    Impl() = default;
    explicit Impl(const std::filesystem::path& source) : warping_document_{source} {}

    size_t num_scaling_steps() const { return warping_document_.getNumScalingSteps(); }
    size_t num_scaling_parameters() const { return warping_document_.getNumScalingParameters(); }

    ModelSpecification warp(const ModelSpecification& model_specification) const
    {
        ScalingCache scaling_cache;
        Model model = model_specification.compile();
        ModelState state = model.initial_state(ModelStateStage::report);
        OPynSimModelStatePair msp{model, state};

        // Collect validation issues, throw an exception if there are any.
        const auto validation_issues = collect_scaling_validation_issues(scaling_cache, msp);
        if (not validation_issues.empty()) {
            throw std::runtime_error{create_validation_issues_error_message(validation_issues)};
        }

        // Perform OpenSim model warp and pack it into a `ModelSpecification`.
        return ModelSpecification{warp_model(scaling_cache, msp)};
    }
private:
    /// Returns a warped version of `source_model` - assumes there are no validation issues.
    OpenSim::Model warp_model(ScalingCache& scaling_cache, const ModelStatePair& msp) const
    {
        OSC_ASSERT(not has_validation_issues(scaling_cache, msp));

        OpenSim::Model rv = msp.getModel();
        rv.clearConnections();
        InitializeModel(rv);
        InitializeState(rv);

        if (not warping_document_.hasScalingSteps()) {
            return rv;  // No `ScalingStep`s, nothing to do.
        }

        // Apply each `ScalingStep` one-by-one.
        const ScalingParameters scaling_parameters = warping_document_.getEffectiveScalingParameters();
        for (const auto& scaling_step : warping_document_.getComponentList<ScalingStep>()) {
            scaling_step.applyScalingStep(scaling_cache, scaling_parameters, msp.getModel(), rv);
        }

        return rv;
    }

    /// Returns a human-readable error message representation of `validation_messages`.
    std::string create_validation_issues_error_message(
        const std::vector<ScalingDocumentValidationMessage>& validation_messages) const
    {
        std::string rv = "Cannot warp `model_specification` due to validation errors:\n";
        for (const auto& validation_message : validation_messages) {
            std::format_to(
                std::back_inserter(rv),
                "- {} ({})",
                validation_message.payload.getMessage(),
                validation_message.sourceScalingStepAbsPath.toString()
            );
        }
        return rv;
    }

    /// Returns all validation messages from all enabled `ScalingStep`s.
    std::vector<ScalingDocumentValidationMessage> collect_scaling_validation_issues(
        ScalingCache& scaling_cache,
        const ModelStatePair& msp) const
    {
        std::vector<ScalingDocumentValidationMessage> rv;

        if (not warping_document_.hasScalingSteps()) {
            return rv;  // No scaling steps, no validation messages.
        }

        const ScalingParameters scaling_params = warping_document_.getEffectiveScalingParameters();
        for (const auto& scaling_step : warping_document_.getComponentList<ScalingStep>()) {
            if (not scaling_step.get_enabled()) {
                continue;  // Only enabled `ScalingStep`s are validated.
            }

            auto messages = scaling_step.validate(scaling_cache, scaling_params, msp);
            rv.reserve(rv.size() + messages.size());
            for (auto& message : messages) {
                rv.push_back(ScalingDocumentValidationMessage{
                    .sourceScalingStepAbsPath = scaling_step.getAbsolutePath(),
                    .payload = std::move(message),
                });
            }
        }

        return rv;
    }

    /// Returns `true` if `msp` has validation issues.
    bool has_validation_issues(ScalingCache& scaling_cache, const ModelStatePair& msp) const
    {
        return not collect_scaling_validation_issues(scaling_cache, msp).empty();
    }

    ModelWarperV3Document warping_document_;
};

ModelWarper opyn::ModelWarper::from_xml(const std::filesystem::path& source)
{
    return ModelWarper{osc::make_cowv<Impl>(source)};
}

opyn::ModelWarper::ModelWarper() :
    impl_{osc::make_cowv<Impl>()}
{}
opyn::ModelWarper::ModelWarper(osc::CopyOnUpdSharedValue<Impl> impl) :
    impl_{std::move(impl)}
{}

size_t opyn::ModelWarper::num_scaling_steps() const      { return impl_->num_scaling_steps(); }
size_t opyn::ModelWarper::num_scaling_parameters() const { return impl_->num_scaling_parameters(); }
ModelSpecification opyn::ModelWarper::warp(const ModelSpecification& model_specification) const
{
    return impl_->warp(model_specification);
}

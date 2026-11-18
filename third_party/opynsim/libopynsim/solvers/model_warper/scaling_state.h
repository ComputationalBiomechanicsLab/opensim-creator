#pragma once

#include <libopynsim/documents/model/object_property_edit.h>
#include <libopynsim/documents/model/basic_model_state_pair.h>
#include <libopynsim/solvers/model_warper/model_warper_v3_document.h>
#include <libopynsim/solvers/model_warper/scaling_document_validation_message.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <filesystem>
#include <format>
#include <memory>
#include <utility>
#include <vector>

namespace opyn
{
    // Top-level input state that's required to actually perform model scaling.
    class ScalingState final {
    public:
        explicit ScalingState()
        {
            scaling_document_->finalizeConnections(*scaling_document_);
            scaling_document_->finalizeFromProperties();
        }

        ScalingState(const ScalingState& other) :
            source_model_{std::make_shared<BasicModelStatePair>(*other.source_model_)},
            scaling_document_{std::make_shared<ModelWarperV3Document>(*other.scaling_document_)}
        {
            // care: separate `ScalingState`s should act like separate instances with no
            //       reference sharing between them, but the shared pointers in the "main"
            //       `ScalingState` might already be divvied out to UI components, so we
            //       can't just switch the pointers around.
            scaling_document_->clearConnections();
            scaling_document_->finalizeConnections(*scaling_document_);
            scaling_document_->finalizeFromProperties();
        }

        ~ScalingState() noexcept = default;

        ScalingState& operator=(const ScalingState& other)
        {
            // care: separate `ScalingState`s should act like separate instances with no
            //       reference sharing between them, but the shared pointers in the "main"
            //       `ScalingState` might already be divvied out to UI components, so we
            //       can't just switch the pointers around.
            if (&other == this) {
                return *this;
            }
            *source_model_ = *other.source_model_;
            *scaling_document_ = *other.scaling_document_;
            scaling_document_->clearConnections();
            scaling_document_->finalizeConnections(*scaling_document_);
            scaling_document_->finalizeFromProperties();
            return *this;
        }

        // Source Model Methods

        const ModelStatePair& get_source_model() const { return *source_model_; }
        std::shared_ptr<ModelStatePair> get_source_model_ptr() { return source_model_; }
        void load_source_model_from_osim(const std::filesystem::path& path)
        {
            source_model_ = std::make_shared<BasicModelStatePair>(path);
        }
        void reset_source_model() { source_model_ = std::make_shared<BasicModelStatePair>(); }

        // Scaling Document Methods

        std::shared_ptr<const ModelWarperV3Document> get_scaling_document_ptr() const
        {
            return scaling_document_;
        }
        bool has_scaling_steps() const { return scaling_document_->has_scaling_steps(); }
        auto iterate_scaling_steps() const { return scaling_document_->iterate_scaling_steps(); }
        void add_scaling_step(std::unique_ptr<ScalingStep> step)
        {
            scaling_document_->add_scaling_step(std::move(step));
        }
        bool erase_scaling_step(ScalingStep& step)
        {
            return scaling_document_->remove_scaling_step(step);
        }
        bool erase_scaling_step(const OpenSim::ComponentPath& path)
        {
            if (auto* scaling_step = find_scaling_component_mut<ScalingStep>(path)) {
                return erase_scaling_step(*scaling_step);
            } else {
                return false;
            }
        }
        void apply_scaling_object_property_edit(osc::ObjectPropertyEdit edit)
        {
            OpenSim::Component* component =
                find_scaling_component_mut(edit.get_component_abs_path());
            if (not component) {
                return;
            }
            OpenSim::AbstractProperty* property =
                find_property_mut(*component, edit.get_property_name());
            if (not property) {
                return;
            }
            edit.apply(*property);
            scaling_document_->clearConnections();
            scaling_document_->finalizeConnections(*scaling_document_);
            scaling_document_->finalizeFromProperties();
        }
        bool disable_scaling_step(const OpenSim::ComponentPath& path)
        {
            if (auto* scaling_step = find_scaling_component_mut<ScalingStep>(path)) {
                scaling_step->set_enabled(false);
                scaling_document_->clearConnections();
                scaling_document_->finalizeConnections(*scaling_document_);
                scaling_document_->finalizeFromProperties();
                return true;
            } else {
                return false;
            }
        }
        std::vector<ScalingDocumentValidationMessage> get_enabled_scaling_step_validation_messages(
            ScalingCache& scaling_cache) const
        {
            std::vector<ScalingDocumentValidationMessage> rv;

            if (not has_scaling_steps()) {
                return rv;
            }

            const ScalingParameters scaling_parameters = get_effective_scaling_parameters();

            for (const auto& scaling_step : scaling_document_->getComponentList<ScalingStep>()) {
                if (not scaling_step.get_enabled()) {
                    // Only aggregate validation errors from enabled `ScalingStep`s at the
                    // document-level.
                    continue;
                }
                auto step_messages =
                    scaling_step.validate(scaling_cache, scaling_parameters, *source_model_);
                rv.reserve(rv.size() + step_messages.size());
                for (auto& step_message : step_messages) {
                    rv.push_back(
                        ScalingDocumentValidationMessage{
                            .source_scaling_step_abs_path = scaling_step.getAbsolutePath(),
                            .payload = std::move(step_message),
                        });
                }
            }
            return rv;
        }
        bool has_scaling_step_validation_issues(ScalingCache& scaling_cache) const
        {
            return not get_enabled_scaling_step_validation_messages(scaling_cache).empty();
        }
        void reset_scaling_document()
        {
            scaling_document_ = std::make_shared<ModelWarperV3Document>();
            scaling_document_->finalizeConnections(*scaling_document_);
            scaling_document_->finalizeFromProperties();
        }
        void load_scaling_document(const std::filesystem::path& path)
        {
            scaling_document_ = std::make_shared<ModelWarperV3Document>(path);
        }
        std::optional<std::filesystem::path> scaling_document_filesystem_location() const
        {
            if (const auto filename = scaling_document_->getDocumentFileName();
                not filename.empty()) {

                return std::filesystem::path{filename};
            } else {
                return std::nullopt;
            }
        }

        bool has_scaling_parameter_declarations() const
        {
            return scaling_document_->has_scaling_parameters();
        }

        ScalingParameters get_effective_scaling_parameters() const
        {
            return scaling_document_->get_effective_scaling_parameters();
        }

        bool set_scaling_parameter_override(
            const std::string& scaling_param_name, ScalingParameterValue new_value)
        {
            return scaling_document_->set_scaling_parameter_override(scaling_param_name, new_value);
        }

        // Model Scaling

        // Tries to generate a scaled version of the source model using the current
        // scaling steps and scaling parameters.
        std::unique_ptr<BasicModelStatePair> try_generate_scaled_model(
            ScalingCache& scaling_cache) const
        {
            if (has_scaling_step_validation_issues(scaling_cache)) {
                return nullptr;  // there are validation errors, so scaling isn't possible
            }

            // Create an independent copy of the source model, which will be scaled in-place.
            OpenSim::Model result_model = source_model_->get_model();
            result_model.clearConnections();
            initialize_model(result_model);
            initialize_state(result_model);

            if (not has_scaling_steps()) {
                // There are no scaling steps, so a copy of the source model is a scaled model
                // (trivially).
                return std::make_unique<BasicModelStatePair>(std::move(result_model));
            }

            // Calculate the effective scaling parameters (defaults + user-enacted overrides)
            const ScalingParameters scaling_params = get_effective_scaling_parameters();

            // Apply each scaling step to the scaled model
            for (const auto& step : scaling_document_->getComponentList<ScalingStep>()) {
                step.apply_scaling_step(
                    scaling_cache, scaling_params, *source_model_, result_model);
            }

            // Return the warped model
            return std::make_unique<BasicModelStatePair>(std::move(result_model));
        }

    private:
        template<typename T = OpenSim::Component>
        T* find_scaling_component_mut(const OpenSim::ComponentPath& p)
        {
            return find_component_mut<T>(*scaling_document_, p);
        }

        std::shared_ptr<BasicModelStatePair> source_model_ =
            std::make_shared<BasicModelStatePair>();
        std::shared_ptr<ModelWarperV3Document> scaling_document_ =
            std::make_shared<ModelWarperV3Document>();
    };
}

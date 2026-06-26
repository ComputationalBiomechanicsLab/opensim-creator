#pragma once

#include <libopynsim/documents/model/object_property_edit.h>
#include <libopynsim/documents/model/basic_model_state_pair.h>
#include <libopynsim/solvers/model_warper/model_warper_v3_document.h>
#include <libopynsim/solvers/model_warper/scaling_document_validation_message.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <filesystem>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace opyn
{
    // Top-level input state that's required to actually perform model scaling.
    class ScalingState final {  // NOLINT(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    public:
        explicit ScalingState()
        {
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
        }

        ScalingState(const ScalingState& other) :
            sourceModel{std::make_shared<BasicModelStatePair>(*other.sourceModel)},
            scalingDocument{std::make_shared<ModelWarperV3Document>(*other.scalingDocument)}
        {
            // care: separate `ScalingState`s should act like separate instances with no
            //       reference sharing between them, but the shared pointers in the "main"
            //       `ScalingState` might already be divvied out to UI components, so we
            //       can't just switch the pointers around.
            scalingDocument->clearConnections();
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
        }

        ScalingState(ScalingState&& tmp) noexcept :
            ScalingState{static_cast<const ScalingState&>(tmp)}
        {}

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
            *sourceModel = *other.sourceModel;
            *scalingDocument = *other.scalingDocument;
            scalingDocument->clearConnections();
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
            return *this;
        }

        ScalingState& operator=(ScalingState&& tmp) noexcept { return *this = static_cast<const ScalingState&>(tmp); }

    // Source Model Methods

        const ModelStatePair& getSourceModel() const { return *sourceModel; }
        std::shared_ptr<ModelStatePair> getSourceModelPtr() { return sourceModel; }
        void loadSourceModelFromOsim(const std::filesystem::path& path)
        {
            sourceModel = std::make_shared<BasicModelStatePair>(path);
        }
        void resetSourceModel()
        {
            sourceModel = std::make_shared<BasicModelStatePair>();
        }

    // Scaling Document Methods

        std::shared_ptr<const ModelWarperV3Document> getScalingDocumentPtr() const { return scalingDocument; }
        bool hasScalingSteps() const { return scalingDocument->hasScalingSteps(); }
        auto iterateScalingSteps() const { return scalingDocument->iterateScalingSteps(); }
        void addScalingStep(std::unique_ptr<ScalingStep> step)
        {
            scalingDocument->addScalingStep(std::move(step));
        }
        bool eraseScalingStep(ScalingStep& step)
        {
            return scalingDocument->removeScalingStep(step);
        }
        bool eraseScalingStep(const OpenSim::ComponentPath& path)
        {
            if (auto* scalingStep = findScalingComponentMut<ScalingStep>(path)) {
                return eraseScalingStep(*scalingStep);
            }
            else {
                return false;
            }
        }
        void applyScalingObjectPropertyEdit(osc::ObjectPropertyEdit edit)
        {
            OpenSim::Component* component = findScalingComponentMut(edit.getComponentAbsPath());
            if (not component) {
                return;
            }
            OpenSim::AbstractProperty* property = FindPropertyMut(*component, edit.getPropertyName());
            if (not property) {
                return;
            }
            edit.apply(*property);
            scalingDocument->clearConnections();
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
        }
        bool disableScalingStep(const OpenSim::ComponentPath& path)
        {
            if (auto* scalingStep = findScalingComponentMut<ScalingStep>(path)) {
                scalingStep->set_enabled(false);
                scalingDocument->clearConnections();
                scalingDocument->finalizeConnections(*scalingDocument);
                scalingDocument->finalizeFromProperties();
                return true;
            }
            else {
                return false;
            }
        }
        std::vector<ScalingDocumentValidationMessage> getEnabledScalingStepValidationMessages(ScalingCache& scalingCache) const
        {
            std::vector<ScalingDocumentValidationMessage> rv;

            if (not hasScalingSteps()) {
                return rv;
            }

            const ScalingParameters scalingParameters = getEffectiveScalingParameters();

            for (const auto& scalingStep : scalingDocument->getComponentList<ScalingStep>()) {
                if (not scalingStep.get_enabled()) {
                    // Only aggregate validation errors from enabled `ScalingStep`s at the document-level.
                    continue;
                }
                auto stepMessages = scalingStep.validate(scalingCache, scalingParameters, *sourceModel);
                rv.reserve(rv.size() + stepMessages.size());
                for (auto& stepMessage : stepMessages) {
                    rv.push_back(ScalingDocumentValidationMessage{
                        .sourceScalingStepAbsPath = scalingStep.getAbsolutePath(),
                        .payload = std::move(stepMessage),
                    });
                }
            }
            return rv;
        }
        bool hasScalingStepValidationIssues(ScalingCache& scalingCache) const
        {
            return not getEnabledScalingStepValidationMessages(scalingCache).empty();
        }
        void resetScalingDocument()
        {
            scalingDocument = std::make_shared<ModelWarperV3Document>();
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
        }
        void loadScalingDocument(const std::filesystem::path& path)
        {
            const auto ptr = std::shared_ptr<OpenSim::Object>{OpenSim::Object::makeObjectFromFile(path.string())};
            if (auto downcasted = std::dynamic_pointer_cast<ModelWarperV3Document>(ptr)) {
                scalingDocument = std::move(downcasted);
                scalingDocument->finalizeConnections(*scalingDocument);
                scalingDocument->finalizeFromProperties();
            }
            else {
                std::stringstream ss;
                ss << path.string() << ": is a valid object file, but doesn't contain a ModelWarperV3Document";
                throw std::runtime_error{std::move(ss).str()};
            }
        }
        std::optional<std::filesystem::path> scalingDocumentFilesystemLocation() const
        {
            if (const auto filename = scalingDocument->getDocumentFileName(); not filename.empty()) {
                return std::filesystem::path{filename};
            }
            else {
                return std::nullopt;
            }
        }

        bool hasScalingParameterDeclarations() const { return scalingDocument->hasScalingParameters(); }
        ScalingParameters getEffectiveScalingParameters() const { return scalingDocument->getEffectiveScalingParameters(); }
        bool setScalingParameterOverride(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            return scalingDocument->setScalingParameterOverride(scalingParamName, newValue);
        }

    // Model Scaling

        // Tries to generate a scaled version of the source model using the current
        // scaling steps and scaling parameters.
        std::unique_ptr<BasicModelStatePair> tryGenerateScaledModel(ScalingCache& scalingCache) const
        {
            if (hasScalingStepValidationIssues(scalingCache)) {
                return nullptr;  // there are validation errors, so scaling isn't possible
            }

            // Create an independent copy of the source model, which will be scaled in-place.
            OpenSim::Model resultModel = sourceModel->getModel();
            resultModel.clearConnections();
            InitializeModel(resultModel);
            InitializeState(resultModel);

            if (not hasScalingSteps()) {
                // There are no scaling steps, so a copy of the source model is a scaled model (trivially).
                return std::make_unique<BasicModelStatePair>(std::move(resultModel));
            }

            // Calculate the effective scaling parameters (defaults + user-enacted overrides)
            const ScalingParameters scalingParams = getEffectiveScalingParameters();

            // Apply each scaling step to the scaled model
            for (auto& step : scalingDocument->updComponentList<ScalingStep>()) {
                step.applyScalingStep(scalingCache, scalingParams, *sourceModel, resultModel);
            }

            // Return the warped model
            return std::make_unique<BasicModelStatePair>(std::move(resultModel));
        }

    private:
        template<typename T = OpenSim::Component>
        T* findScalingComponentMut(const OpenSim::ComponentPath& p) { return FindComponentMut<T>(*scalingDocument, p); }

        std::shared_ptr<BasicModelStatePair> sourceModel = std::make_shared<BasicModelStatePair>();
        std::shared_ptr<ModelWarperV3Document> scalingDocument = std::make_shared<ModelWarperV3Document>();
    };
}

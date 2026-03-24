#include "model_warper_tab.h"

#include <libopensimcreator/documents/file_filters.h>
#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/recent_files.h>
#include <libopensimcreator/ui/model_editor/model_editor_tab.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/main_menu.h>
#include <libopensimcreator/ui/shared/model_viewer_panel.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_parameters.h>
#include <libopensimcreator/ui/shared/object_properties_editor.h>

#include <libopynsim/documents/custom_components/in_memory_mesh.h>
#include <libopynsim/documents/landmarks/landmark_helpers.h>
#include <libopynsim/documents/landmarks/maybe_named_landmark_pair.h>
#include <libopynsim/documents/model/simple_model_state_pair.h>
#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/solvers/model_warper/all_scaling_step_types.h>
#include <libopynsim/solvers/model_warper/model_warper_v3_document.h>
#include <libopynsim/solvers/model_warper/scaling_document_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_state.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <libopynsim/tps3d.h>
#include <liboscar/formats/obj.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/transform_functions.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/events/reset_ui_context_event.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/panels/panel_manager.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/ui/widgets/redo_button.h>
#include <liboscar/ui/widgets/undo_button.h>
#include <liboscar/ui/widgets/window_menu.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/typelist.h>
#include <liboscar/utilities/undo_redo.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapCylinder.h>
#include <OpenSim/Simulation/Wrap/WrapEllipsoid.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace osc;
using namespace opyn;

// Controller datastructures (i.e. middleware between the UI and the underlying model).
namespace
{
    // Top-level UI state that's shared between panels/widget that the UI manipulates.
    class ModelWarperV3UIState final : public std::enable_shared_from_this<ModelWarperV3UIState> {
    public:

        explicit ModelWarperV3UIState(Widget* parent) :
            parent_{parent}
        {}

        // Should be regularly called by the UI once per frame.
        void on_tick()
        {
            try {
                if (not m_DeferredActions.empty()) {
                    for (auto& deferredAction : m_DeferredActions) {
                        deferredAction(*this);
                    }
                    m_DeferredActions.clear();

                    updateScaledModel();
                }
            }
            catch (const std::exception& ex) {
                log_error("error processing deferred actions: %s", ex.what());
            }
        }

        // Returns a shared readonly pointer to the top-level model warping document.
        std::shared_ptr<const ModelWarperV3Document> getDocumentPtr() const
        {
            return m_ScalingState->scratch().getScalingDocumentPtr();
        }


        bool hasScalingSteps() const
        {
            return m_ScalingState->scratch().hasScalingSteps();
        }
        auto iterateScalingSteps() const
        {
            return m_ScalingState->scratch().iterateScalingSteps();
        }
        void addScalingStepDeferred(std::unique_ptr<ScalingStep> step)
        {
            m_DeferredActions.emplace_back([s = std::shared_ptr<ScalingStep>{std::move(step)}](ModelWarperV3UIState& state) mutable
            {
                state.m_ScalingState->upd_scratch().addScalingStep(std::unique_ptr<ScalingStep>{s->clone()});
                state.m_ScalingState->commit_scratch("Add scaling step");
            });
        }
        void eraseScalingStepDeferred(const ScalingStep& step)
        {
            m_DeferredActions.emplace_back([path = step.getAbsolutePath()](ModelWarperV3UIState& state)
            {
                if (state.m_ScalingState->upd_scratch().eraseScalingStep(path)) {
                    state.m_ScalingState->commit_scratch("Erase scaling step");
                }
            });
        }
        std::vector<ScalingStepValidationMessage> validateStep(const ScalingStep& step)
        {
            return step.validate(
                m_ScalingCache,
                m_ScalingState->scratch().getEffectiveScalingParameters(),
                m_ScalingState->scratch().getSourceModel()
            );
        }


        bool hasScalingParameters() const
        {
            return m_ScalingState->scratch().hasScalingParameterDeclarations();
        }
        ScalingParameters getEffectiveScalingParameters() const
        {
            return m_ScalingState->scratch().getEffectiveScalingParameters();
        }
        void setScalingParameterValueDeferred(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            m_DeferredActions.emplace_back([scalingParamName, newValue](ModelWarperV3UIState& state)
            {
                if (state.m_ScalingState->upd_scratch().setScalingParameterOverride(scalingParamName, newValue)) {
                    state.m_ScalingState->commit_scratch("Set scaling parameter");
                }
            });
        }


        std::shared_ptr<ModelStatePair> sourceModel()
        {
            return m_ScalingState->upd_scratch().getSourceModelPtr();
        }


        // result model stuff (note: might not be available if there's validation issues)
        using ScaledModelOrValidationErrorsOrScalingErrors = std::variant<
            std::shared_ptr<ModelStatePair>,                // successfully scaled model
            std::vector<ScalingDocumentValidationMessage>,  // validation messages
            std::string                                     // runtime scaling error message (i.e. after validation passed but during scaling)
        >;
        ScaledModelOrValidationErrorsOrScalingErrors scaledModelOrDocumentValidationMessages()
        {
            if (m_ScalingErrorMessage) {
                return m_ScalingErrorMessage.value();
            }
            else if (auto validationMessages = m_ScalingState->scratch().getEnabledScalingStepValidationMessages(m_ScalingCache); not validationMessages.empty()) {
                return validationMessages;
            }
            else {
                return m_ScaledModel;
            }
        }

        bool canExportWarpedModel() const
        {
            return not m_ScalingErrorMessage.has_value();
        }

        void exportWarpedModelToModelEditor()
        {
            if (not canExportWarpedModel()) {
                return;  // can't warp
            }

            auto scalingResult = scaledModelOrDocumentValidationMessages();
            if (not std::holds_alternative<std::shared_ptr<ModelStatePair>>(scalingResult)) {
                log_error("cannot export scaled model: could not create a warped model");
                return;
            }

            const OpenSim::Model& sourceModel = m_ScalingState->scratch().getSourceModel();
            const auto modelFilesystemLocation = TryFindInputFile(sourceModel);
            if (not modelFilesystemLocation) {
                log_error("cannot export scaled model: can't figure out where the source model is on-disk");
                return;
            }

            std::shared_ptr<ModelStatePair> scaled = std::get<std::shared_ptr<ModelStatePair>>(scalingResult);

            std::unique_ptr<OpenSim::Model> copy = std::make_unique<OpenSim::Model>(scaled->getModel());
            InitializeModel(*copy);
            InitializeState(*copy);
            {
                // TODO/FIXME/HACK: this code was thrown together to solve an immediate problem
                // of being able to export warped models, but it isn't very clean or robust (#1003).
                auto inMemoryMeshes = scaled->getModel().getComponentList<InMemoryMesh>();
                if (inMemoryMeshes.begin() != inMemoryMeshes.end()) {
                    const auto warpedGeometryDir = tryGetWarpedGeometryDirectory();
                    if (not warpedGeometryDir) {
                        log_error("cannot export scaled model: can't figure out where to save the warped meshes");
                        return;
                    }

                    // If the warped geometry directory doesn't exist yet, create it.
                    if (not std::filesystem::exists(*warpedGeometryDir)) {
                        std::filesystem::create_directories(*warpedGeometryDir);
                    }

                    // Export `InMemoryMesh`es to disk (#1003)
                    for (const InMemoryMesh& mesh : inMemoryMeshes) {
                        // Figure out output file name.
                        const auto& inputMesh = sourceModel.getComponent<OpenSim::Mesh>(mesh.getAbsolutePath());
                        const auto warpedMeshAbsPath = std::filesystem::weakly_canonical(*warpedGeometryDir / std::filesystem::path{inputMesh.get_mesh_file()}.filename().replace_extension(".obj"));

                        // Write warped mesh data to disk in an OBJ format.
                        {
                            std::ofstream objStream{warpedMeshAbsPath, std::ios::trunc};
                            objStream.exceptions(std::ios::badbit | std::ios::failbit);
                            OBJ::write(objStream, mesh.getOscMesh(), OBJMetadata{"osc-model-warper"});
                        }

                        // Overwrite the `InMemoryMesh` in `copy`
                        auto& copyMesh = copy->updComponent<InMemoryMesh>(mesh.getAbsolutePath());
                        auto newMesh = std::make_unique<OpenSim::Mesh>();
                        newMesh->set_mesh_file(warpedMeshAbsPath.string());
                        OverwriteGeometry(*copy, copyMesh, std::move(newMesh));
                    }
                }
            }

            // Ensure the scaled model doesn't refer to the same filepath as the source
            // model, because it's  quite dangerous if the user can save over their source
            // model easily (#1146).
            copy->setInputFileName("");

            // Make the model undo-able
            auto undoableModel = std::make_unique<UndoableModelStatePair>(std::move(copy));

            // If the user requests it, bake the SDFs after finishing everything else
            if (getBakeSDFs()) {
                ActionBakeStationDefinedFrames(*undoableModel);
            }

            auto editor = std::make_unique<ModelEditorTab>(parent_, std::move(undoableModel));
            App::post_event<OpenTabEvent>(*parent_, std::move(editor));
        }

        // camera stuff
        bool isCameraLinked() const { return m_LinkCameras; }
        void setCameraLinked(bool v) { m_LinkCameras = v; }
        bool isOnlyCameraRotationLinked() const { return m_OnlyLinkRotation; }
        void setOnlyCameraRotationLinked(bool v) { m_OnlyLinkRotation = v; }
        const PolarPerspectiveCamera& getLinkedCamera() const { return m_LinkedCamera; }
        void setLinkedCamera(const PolarPerspectiveCamera& camera) { m_LinkedCamera = camera; }


        // undo/redo stuff
        std::shared_ptr<UndoRedoBase> getUndoRedoPtr() { return m_ScalingState; }


        // warped geometry dir stuff
        std::optional<std::filesystem::path> tryGetWarpedGeometryDirectory() const
        {
            if (m_MaybeCustomWarpedGeometryDirectory) {
                return m_MaybeCustomWarpedGeometryDirectory;  // top-prio is user-enacted choice (#1046).
            }

            const OpenSim::Model& sourceModel = m_ScalingState->scratch().getSourceModel();
            const auto modelFilesystemLocation = TryFindInputFile(sourceModel);
            if (modelFilesystemLocation) {
                return modelFilesystemLocation->parent_path() / "WarpedGeometry";
            }

            return std::nullopt;  // can't figure out where to save it :(
        }

        // SDF baking stuff
        bool getBakeSDFs() const { return m_BakeStationDefinedFrames; }
        void setBakeSDFs(bool v) { m_BakeStationDefinedFrames = v; }

        // actions
        void actionCreateNewSourceModel()
        {
            m_ScalingState->upd_scratch().resetSourceModel();
            updateScaledModel();
            m_ScalingState->commit_scratch("Create new source model");
        }

        void actionOpenOsimOrPromptUser(std::optional<std::filesystem::path> path)
        {
            if (path) {
                actionOpenOsim(*path);
            }
            else {
                if (not shared_from_this()) {
                    log_critical("can't open osim file selection dialog because the UI state isn't shared");
                    return;
                }

                App::upd().prompt_user_to_select_file_async(
                    [state = shared_from_this()](const FileDialogResponse& response)
                    {
                        if (not state) {
                            return;  // Something bad has happened.
                        }
                        if (response.size() != 1) {
                            return;  // Error, cancellation, or the user somehow selected >1 file.
                        }
                        state->actionOpenOsim(response.front());
                    },
                    GetModelFileFilters()
                );
            }
        }

        void actionPromptUserToSelectWarpedGeometryDirectory()
        {
            App::upd().prompt_user_to_select_directory_async(
                [state = shared_from_this()](const FileDialogResponse& response)
                {
                    if (not state) {
                        return;  // Can't continue.
                    }
                    if (response.size() != 1) {
                        return;  // Error, cancellation, or the user somehow selected >1 file.
                    }

                    state->m_MaybeCustomWarpedGeometryDirectory = response.front();
                }
            );
        }

        void actionOpenOsim(const std::filesystem::path& path)
        {
            App::singleton<RecentFiles>()->push_back(path);
            m_ScalingState->upd_scratch().loadSourceModelFromOsim(path);
            updateScaledModel();
            m_ScalingState->commit_scratch("Loaded osim file");
        }

        void actionCreateNewScalingDocument()
        {
            m_ScalingState->upd_scratch().resetScalingDocument();
            updateScaledModel();
            m_ScalingState->commit_scratch("Create new scaling document");
        }

        void actionOpenScalingDocument()
        {
            if (not shared_from_this()) {
                log_critical("cannot open a file selection dialog because the UI state isn't reference-counted");
                return;
            }

            App::upd().prompt_user_to_select_file_async(
                [state = shared_from_this()](const FileDialogResponse& response)
                {
                    if (not state) {
                        return;  // Can't continue.
                    }
                    if (response.size() != 1) {
                        return;  // Error, cancellation, or the user somehow selected >1 file.
                    }

                    state->m_ScalingState->upd_scratch().loadScalingDocument(response.front());
                    state->updateScaledModel();
                    state->m_ScalingState->commit_scratch("Loaded scaling document");
                },
                GetOpenSimXMLFileFilters()
            );
        }

        void actionSaveScalingDocument()
        {
            if (const auto existingPath = m_ScalingState->scratch().scalingDocumentFilesystemLocation()) {
                m_ScalingState->upd_scratch().getScalingDocumentPtr()->saveTo(*existingPath);
                return;  // Document saved to existing filesystem location.
            }

            // Else: prompt the user to save it
            App::upd().prompt_user_to_save_file_with_extension_async([doc = m_ScalingState->upd_scratch().getScalingDocumentPtr()](std::optional<std::filesystem::path> p)
            {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }
                doc->saveTo(*p);
            }, "xml");
        }

        void actionApplyObjectEditToScalingDocument(const ObjectPropertyEdit& edit)
        {
            m_ScalingState->upd_scratch().applyScalingObjectPropertyEdit(edit);
            updateScaledModel();
            m_ScalingState->commit_scratch("change scaling property");
        }

        void actionDisableScalingStep(const OpenSim::ComponentPath& path)
        {
            m_ScalingState->upd_scratch().disableScalingStep(path);
            updateScaledModel();
            m_ScalingState->commit_scratch("disable scaling step");
        }

        void actionRollback()
        {
            m_ScalingState->rollback();
        }

        void actionRetryScalingDeferred()
        {
            m_DeferredActions.emplace_back([](ModelWarperV3UIState& state) mutable
            {
                state.updateScaledModel();
            });
        }

        bool canUndo() const
        {
            return m_ScalingState->can_undo();
        }
        void actionUndo()
        {
            m_ScalingState->undo();
        }
    private:
        void updateScaledModel()
        {
            try {
                auto scaledModel = m_ScalingState->scratch().tryGenerateScaledModel(m_ScalingCache);
                if (not scaledModel) {
                    return;
                }

                m_ScaledModel = std::move(scaledModel);
                m_ScalingErrorMessage.reset();
            }
            catch (const std::exception& ex) {
                m_ScalingErrorMessage = ex.what();
            }
        }

        Widget* parent_ = nullptr;

        std::shared_ptr<UndoRedo<ScalingState>> m_ScalingState = std::make_shared<UndoRedo<ScalingState>>();

        ScalingCache m_ScalingCache;
        std::shared_ptr<SimpleModelStatePair> m_ScaledModel = std::make_shared<SimpleModelStatePair>();
        std::optional<std::string> m_ScalingErrorMessage;

        std::vector<std::function<void(ModelWarperV3UIState&)>> m_DeferredActions;
        bool m_LinkCameras = true;
        bool m_OnlyLinkRotation = false;
        PolarPerspectiveCamera m_LinkedCamera;

        std::optional<std::filesystem::path> m_MaybeCustomWarpedGeometryDirectory;
        bool m_BakeStationDefinedFrames = false;
    };
}

namespace
{
    Color ui_color(const ScalingStepValidationMessage& message)
    {
        switch (message.getState()) {
        case ScalingStepValidationState::Warning: return Color::orange();
        case ScalingStepValidationState::Error:   return Color::muted_red();
        default:                                  return Color::muted_red();
        }
    }

    // source model 3D viewer
    class ModelWarperV3SourceModelViewerPanel final : public ModelViewerPanel {
    public:
        explicit ModelWarperV3SourceModelViewerPanel(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<ModelWarperV3UIState> state) :

            ModelViewerPanel{parent, label, ModelViewerPanelParameters{state->sourceModel()}, ModelViewerPanelFlag::NoHittest},
            m_State{std::move(state)}
        {}

    private:
        void impl_draw_content() final
        {
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = getCamera();
                    camera.phi = m_State->getLinkedCamera().phi;
                    camera.theta = m_State->getLinkedCamera().theta;
                    setCamera(camera);
                }
                else {
                    setCamera(m_State->getLinkedCamera());
                }
            }

            setModelState(m_State->sourceModel());
            ModelViewerPanel::impl_draw_content();

            // draw may have updated the camera, so flash is back
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = m_State->getLinkedCamera();
                    camera.phi = getCamera().phi;
                    camera.theta = getCamera().theta;
                    m_State->setLinkedCamera(camera);
                }
                else {
                    m_State->setLinkedCamera(getCamera());
                }
            }
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
    };

    // result model 3D viewer
    class ModelWarperV3ResultModelViewerPanel final : public ModelViewerPanel {
    public:
        ModelWarperV3ResultModelViewerPanel(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<ModelWarperV3UIState> state) :

            ModelViewerPanel{parent, label, ModelViewerPanelParameters{state->sourceModel()}, ModelViewerPanelFlag::NoHittest},
            m_State{std::move(state)}
        {}

    private:
        void impl_draw_content() final
        {
            std::visit(Overload{
                [this](const std::shared_ptr<ModelStatePair>& scaledModel) { draw_scaled_model_visualization(scaledModel); },
                [this](const std::vector<ScalingDocumentValidationMessage>& messages) { draw_validation_error_message(messages); },
                [this](const std::string& scalingErrorMessage) { draw_scaling_error_message(scalingErrorMessage); },
            }, m_State->scaledModelOrDocumentValidationMessages());
        }

        void draw_scaled_model_visualization(const std::shared_ptr<ModelStatePair>& scaledModel)
        {
            // handle camera linking
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = getCamera();
                    camera.phi = m_State->getLinkedCamera().phi;
                    camera.theta = m_State->getLinkedCamera().theta;
                    setCamera(camera);
                }
                else {
                    setCamera(m_State->getLinkedCamera());
                }
            }

            setModelState(scaledModel);
            ModelViewerPanel::impl_draw_content();

            // draw may have updated the camera, so flash is back
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = m_State->getLinkedCamera();
                    camera.phi = getCamera().phi;
                    camera.theta = getCamera().theta;
                    m_State->setLinkedCamera(camera);
                }
                else {
                    m_State->setLinkedCamera(getCamera());
                }
            }
        }

        void draw_validation_error_message(std::span<const ScalingDocumentValidationMessage> messages)
        {
            const float contentHeight = static_cast<float>(messages.size() + 2) * ui::get_text_line_height_in_current_panel();
            const float regionHeight = ui::get_content_region_available().y();
            const float top = 0.5f * (regionHeight - contentHeight);

            ui::set_cursor_panel_position({0.0f, top});

            // header line
            {
                std::stringstream ss;
                ss << "Cannot show model: " << messages.size() << " validation error" << (messages.size() > 1 ? "s" : "") << " detected:";
                ui::draw_text_centered(std::move(ss).str());
            }

            // error line(s)
            int id = 0;
            for (const auto& message : messages) {
                ui::push_id(id++);

                ui::push_style_color(ui::ColorVar::Text, ui_color(message.payload));
                std::stringstream ss;
                ss << message.sourceScalingStepAbsPath.getComponentName() << ": " << message.payload.getMessage();
                ui::draw_text_bullet_pointed(std::move(ss).str());
                ui::pop_style_color();

                ui::same_line();
                if (ui::draw_small_button("Disable Scaling Step")) {
                    m_State->actionDisableScalingStep(message.sourceScalingStepAbsPath);
                }

                ui::pop_id();
            }
        }

        void draw_scaling_error_message(CStringView message)
        {
            const float h = ui::get_content_region_available().y();
            const float lineHeight = ui::get_text_line_height_in_current_panel();
            constexpr float numLines = 3.0f;
            const float top = 0.5f * (h - numLines*lineHeight);

            ui::set_cursor_panel_position({0.0f, top});
            ui::draw_text_centered("An error occured while trying to scale the model:");
            ui::draw_text_centered(message);
            if (ui::draw_button_centered(MSMICONS_RECYCLE " Retry Scaling")) {
                m_State->actionRetryScalingDeferred();
            }
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
    };

    class WarpedModelExporterPopup final : public Popup {
    public:
        explicit WarpedModelExporterPopup(Widget* parent) :
            Popup{parent, "hello, popup world!"}
        {}

    private:
        void impl_draw_content() final
        {
            ui::draw_text("hello, popup content!");
        }
    };

    // main toolbar
    class ModelWarperV3Toolbar final : public Widget {
    public:
        explicit ModelWarperV3Toolbar(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<ModelWarperV3UIState> state) :
            Widget{parent},
            m_State{std::move(state)}
        {
            set_name(label);
        }
    private:
        void impl_on_draw() final
        {
            if (BeginToolbar(name())) {
                draw_content();
            }
            ui::end_panel();
        }

        void draw_content()
        {
            int id = 0;

            ui::push_id(id++);
            ui::draw_vertical_separator();
            ui::same_line();
            ui::draw_text("Source Model: ");
            ui::same_line();
            if (ui::draw_button(MSMICONS_FILE)) {
                m_State->actionCreateNewSourceModel();
            }
            ui::same_line();
            DrawOpenModelButtonWithRecentFilesDropdown([this](auto maybeSelection)
            {
                m_State->actionOpenOsimOrPromptUser(std::move(maybeSelection));
            });
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();


            ui::push_id(id++);
            ui::same_line();
            ui::draw_text("Scaling Document: ");
            ui::same_line();
            if (ui::draw_button(MSMICONS_FILE)) {
                m_State->actionCreateNewScalingDocument();
            }
            ui::same_line();
            if (ui::draw_button(MSMICONS_FOLDER_OPEN)) {
                m_State->actionOpenScalingDocument();
            }
            ui::same_line();
            if (ui::draw_button(MSMICONS_SAVE)) {
                m_State->actionSaveScalingDocument();
            }
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();


            ui::push_id(id++);
            ui::same_line();
            m_UndoButton.on_draw();
            ui::pop_id();
            ui::push_id(id++);
            ui::same_line();
            m_RedoButton.on_draw();
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();

            // Draw green "Warp Model" button
            {
                ui::push_id(id++);
                ui::same_line();
                bool disabled = false;
                if (not m_State->canExportWarpedModel()) {
                    ui::begin_disabled();
                    disabled = true;
                }

                ui::push_style_color(ui::ColorVar::Button, Color::dark_green());
                if (ui::draw_button(MSMICONS_PLAY " Export Warped Model")) {
                    m_State->exportWarpedModelToModelEditor();
                }
                ui::add_screenshot_annotation_to_last_drawn_item("Export Warped Model Button");
                ui::pop_style_color();
                ui::same_line(0.0f, 1.0f);
                if (ui::draw_button(MSMICONS_COG)) {
                    ui::open_popup("##WarpedModelExportModifiers");
                }


                if (ui::begin_popup("##WarpedModelExportModifiers")) {

                    // UI for warped geometry directory toggle
                    const auto geomDir = m_State->tryGetWarpedGeometryDirectory();
                    if (geomDir) {
                        ui::draw_text("Warped Geometry Directory: %s", geomDir->string().c_str());
                    }
                    else {
                        ui::draw_text("Warped Geometry Directory: UNKNOWN");
                    }
                    ui::same_line();
                    if (ui::draw_small_button("change")) {
                        m_State->actionPromptUserToSelectWarpedGeometryDirectory();
                    }


                    // UI for baking `StationDefinedFrame`s during warping
                    {
                        bool bakeSDFs = m_State->getBakeSDFs();
                        if (ui::draw_checkbox("Bake StationDefinedFrames", &bakeSDFs)) {
                            m_State->setBakeSDFs(bakeSDFs);
                        }
                    }

                    ui::end_popup();
                }
                if (disabled) {
                    ui::end_disabled();
                }
                ui::same_line();
                ui::draw_vertical_separator();
                ui::pop_id();
            }

            ui::push_id(id++);
            ui::same_line();
            if (bool v = m_State->isCameraLinked(); ui::draw_checkbox("link cameras", &v)) {
                m_State->setCameraLinked(v);
            }

            ui::same_line();
            if (bool v = m_State->isOnlyCameraRotationLinked(); ui::draw_checkbox("only link rotation", &v)) {
                m_State->setOnlyCameraRotationLinked(v);
            }
            ui::pop_id();
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
        UndoButton m_UndoButton{this, m_State->getUndoRedoPtr(), MSMICONS_UNDO};
        RedoButton m_RedoButton{this, m_State->getUndoRedoPtr(), MSMICONS_REDO};
    };

    // control panel (design, set parameters, etc.)
    class ModelWarperV3ControlPanel final : public Panel {
    public:
        explicit ModelWarperV3ControlPanel(
            Widget* parent,
            std::string_view panelName,
            std::shared_ptr<ModelWarperV3UIState> state) :

            Panel{parent, panelName},
            m_State{std::move(state)}
        {}

    private:
        void impl_draw_content() final
        {
            draw_scaling_parameters();
            ui::draw_vertical_spacer(0.75f);
            draw_scaling_steps();
        }

        void draw_scaling_parameters()
        {
            ui::draw_text_centered("Scaling Parameters");
            ui::draw_separator();
            ui::draw_vertical_spacer(0.5f);
            if (m_State->hasScalingParameters()) {
                if (ui::begin_table("##ScalingParameters", 2)) {
                    ui::table_setup_column("Name");
                    ui::table_setup_column("Value");
                    ui::table_headers_row();

                    int id = 0;
                    for (const auto& [name, value] : m_State->getEffectiveScalingParameters()) {
                        ui::push_id(id++);
                        ui::table_next_row();
                        ui::table_set_column_index(0);
                        ui::draw_text(name);
                        ui::table_set_column_index(1);
                        auto valueCopy{value};
                        ui::draw_double_input("##valueeditor", &valueCopy, 0.0, 0.0, "%.6f");
                        if (ui::is_item_deactivated_after_edit()) {
                            m_State->setScalingParameterValueDeferred(name, valueCopy);
                        }
                        ui::pop_id();
                    }
                    ui::end_table();
                }
            }
            else {
                ui::draw_text_disabled_and_centered("No Scaling Parameters.");
                ui::draw_text_disabled_and_centered("(scaling parameters are normally implicitly added by scaling steps)");
            }
        }

        void draw_scaling_steps()
        {
            ui::draw_text_centered("Scaling Steps");
            ui::draw_separator();
            ui::draw_vertical_spacer(0.5f);

            if (m_State->hasScalingSteps()) {
                size_t i = 0;
                for (const ScalingStep& step : m_State->iterateScalingSteps()) {
                    ui::push_id(step.getAbsolutePathString());
                    draw_scaling_step(i, step);
                    ui::pop_id();
                    ++i;
                }
            }
            else {
                ui::draw_text_disabled_and_centered("No scaling steps.");
                ui::draw_text_disabled_and_centered("(the model will be left unmodified)");
            }

            ui::draw_vertical_spacer(0.25f);
            draw_add_scaling_step_context_button();
        }

        void draw_scaling_step(size_t stepIndex, const ScalingStep& step)
        {
            // draw collapsing header, don't render content if it's collapsed
            {
                std::stringstream header;
                header << '#' << stepIndex + 1 << ": " << step.label();
                if (not ui::draw_collapsing_header(std::move(header).str(), ui::TreeNodeFlag::DefaultOpen)) {
                    return;  // header is collapsed
                }
            }
            // else: header isn't collapsed

            ui::draw_help_marker(step.getDescription());

            // draw deletion button
            {
                constexpr auto deletionButtonIcon = MSMICONS_TRASH;

                ui::same_line();

                const Vector2 oldCursorPos = ui::get_cursor_panel_position();
                const float endX = oldCursorPos.x() + ui::get_content_region_available().x();

                const Vector2 newCursorPos = {endX - ui::calc_button_size(deletionButtonIcon).x(), oldCursorPos.y()};
                ui::set_cursor_panel_position(newCursorPos);
                if (ui::draw_small_button(deletionButtonIcon)) {
                    m_State->eraseScalingStepDeferred(step);
                }
            }

            // draw validation messages
            {
                const auto messages = m_State->validateStep(step);
                if (not messages.empty()) {
                    ui::draw_vertical_spacer(0.2f);
                    ui::indent();
                    for (const ScalingStepValidationMessage& message : messages) {
                        ui::push_style_color(ui::ColorVar::Text, ui_color(message));
                        ui::draw_bullet_point();
                        if (const auto propName = message.tryGetPropertyName()) {
                            ui::draw_text("%s: %s", propName->c_str(), message.getMessage());
                        }
                        else {
                            ui::draw_text(message.getMessage());
                        }
                        ui::pop_style_color();
                    }
                    ui::unindent();
                    ui::draw_vertical_spacer(0.2f);
                }
            }

            // draw property editors
            ui::indent(1.0f*ui::get_text_line_height_in_current_panel());
            {
                const auto path = step.getAbsolutePathString();
                const auto docPtr = m_State->getDocumentPtr();
                const auto [it, inserted] = m_StepPropertyEditors.try_emplace(
                    path,
                    this,
                    docPtr,
                    [docPtr, path] { return FindComponent(*docPtr, path); }
                );
                if (inserted) {
                    it->second.insertInBlacklist("components");
                }
                if (auto objectEdit = it->second.onDraw()) {
                    m_State->actionApplyObjectEditToScalingDocument(std::move(objectEdit).value());
                }
            }
            ui::unindent(1.0f*ui::get_text_line_height_in_current_panel());
            ui::draw_vertical_spacer(0.5f);
        }

        void draw_add_scaling_step_context_button()
        {
            ui::draw_button(MSMICONS_PLUS "Add Scaling Step", {ui::get_content_region_available().x(), ui::calc_button_size("").y()});
            if (ui::begin_popup_context_menu("##AddScalingStepPopupMenu", ui::PopupFlag::MouseButtonLeft)) {
                for (const auto& ptr : getScalingStepPrototypes()) {
                    ui::push_id(ptr.get());
                    if (ui::draw_selectable(ptr->label())) {
                        m_State->addScalingStepDeferred(std::unique_ptr<ScalingStep>{ptr->clone()});
                    }
                    ui::draw_tooltip_if_item_hovered(ptr->label(), ptr->getDescription(), ui::HoveredFlag::DelayNormal);
                    ui::pop_id();
                }
                ui::end_popup();
            }
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
        std::unordered_map<std::string, ObjectPropertiesEditor> m_StepPropertyEditors;
    };
}

class osc::ModelWarperTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "OpenSim/ModelWarperV3"; }

    explicit Impl(Tab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        // Ensure `ModelWarperV3Document` can be loaded from the filesystem via OpenSim.
        [[maybe_unused]] static const bool s_TypesRegistered = []<typename... TScalingStep>(Typelist<TScalingStep...>)
        {
            OpenSim::Object::registerType(ScalingParameterOverride{});
            (OpenSim::Object::registerType(TScalingStep{}), ...);
            OpenSim::Object::registerType(ModelWarperV3Document{});
            return true;
        }(AllScalingStepTypes{});

        m_PanelManager->register_toggleable_panel("Control Panel", [state = m_State](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<ModelWarperV3ControlPanel>(parent, panelName, state);
        });
        m_PanelManager->register_toggleable_panel("Source Model", [state = m_State](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<ModelWarperV3SourceModelViewerPanel>(parent, panelName, state);
        });
        m_PanelManager->register_toggleable_panel("Result Model", [state = m_State](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<ModelWarperV3ResultModelViewerPanel>(parent, panelName, state);
        });
        m_PanelManager->register_toggleable_panel("Log", [](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<LogViewerPanel>(parent, panelName);
        });
        m_PanelManager->register_toggleable_panel("Performance", [](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<PerfPanel>(parent, panelName);
        });
    }

    void on_mount()
    {
        m_PanelManager->on_mount();
    }

    void on_unmount()
    {
        m_PanelManager->on_unmount();
    }

    void on_tick()
    {
        m_State->on_tick();
        m_PanelManager->on_tick();
    }

    void on_draw_main_menu()
    {
        m_WindowMenu.on_draw();
        m_AboutTab.onDraw();
    }

    void on_draw()
    {
        try {
            ui::enable_dockspace_over_main_window();
            m_PanelManager->on_draw();
            m_Toolbar.on_draw();
            m_ExceptionThrownLastFrame = false;
        } catch (const std::exception& ex) {
            log_error("an exception was thrown, probably due to an error in the document: %s", ex.what());

            // The exception might've left the UI context in an invalid state, so reset it.
            log_error("resetting the UI");
            App::notify<ResetUIContextEvent>(*parent());

            if (std::exchange(m_ExceptionThrownLastFrame, true)) {
                // An exception was also thrown last frame - try to undo the state to try
                // and fix the problem for the user.
                if (m_State->canUndo()) {
                    log_error("attempting to fix the problem by undo-ing the document");
                    m_State->actionUndo();
                }
                else {
                    // Undoing isn't possible, so we must defer this error to a higher level
                    // of the system.
                    log_critical("the document cannot be undone, elevating the exception");
                    throw;
                }
            }
            else {
                // No exception thrown last frame, it's likely that the exception is due to
                // an edit to the scratch space last frame, so try to softly roll the model
                // back.
                log_error("rolling back the document");
                m_State->actionRollback();
            }
        }
    }

private:
    std::shared_ptr<ModelWarperV3UIState> m_State = std::make_shared<ModelWarperV3UIState>(&this->owner());

    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>(&owner());
    WindowMenu m_WindowMenu{&owner(), m_PanelManager};
    MainMenuAboutTab m_AboutTab;
    ModelWarperV3Toolbar m_Toolbar{&this->owner(), "##ModelWarperV3Toolbar", m_State};

    bool m_ExceptionThrownLastFrame = false;
};

CStringView osc::ModelWarperTab::id() { return Impl::static_label(); }

osc::ModelWarperTab::ModelWarperTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::ModelWarperTab::impl_on_mount() { private_data().on_mount(); }
void osc::ModelWarperTab::impl_on_unmount() { private_data().on_unmount(); }
void osc::ModelWarperTab::impl_on_tick() { private_data().on_tick(); }
void osc::ModelWarperTab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::ModelWarperTab::impl_on_draw() { private_data().on_draw(); }

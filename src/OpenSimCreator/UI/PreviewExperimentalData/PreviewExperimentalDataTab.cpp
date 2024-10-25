#include "PreviewExperimentalDataTab.h"

#include <OpenSimCreator/Documents/ExperimentalData/AnnotatedMotion.h>
#include <OpenSimCreator/Documents/ExperimentalData/FileBackedStorage.h>
#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Graphics/MuscleColoringStyle.h>
#include <OpenSimCreator/UI/Events/OpenComponentContextMenuEvent.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/ComponentContextMenu.h>
#include <OpenSimCreator/UI/Shared/CoordinateEditorPanel.h>
#include <OpenSimCreator/UI/Shared/ModelStatusBar.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanel.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanelParameters.h>
#include <OpenSimCreator/UI/Shared/ModelViewerPanelRightClickEvent.h>
#include <OpenSimCreator/UI/Shared/NavigatorPanel.h>
#include <OpenSimCreator/UI/Shared/OutputWatchesPanel.h>
#include <OpenSimCreator/UI/Shared/PropertiesPanel.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Object.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Maths/ClosedInterval.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Events.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/UI/IconCache.h>
#include <oscar/UI/oscimgui.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <span>
#include <vector>

using namespace osc;

// Top-level UI state that's share-able between various panels in the
// preview experimental data UI.
namespace
{
    class PreviewExperimentalDataUiState final {
    public:
        std::shared_ptr<IModelStatePair> updSharedModelPtr() const { return m_Model; }
        IModelStatePair& updModel() { return *m_Model; }

        void on_tick()
        {
            // ensure the model is scrubbed to the current scrub time
            //
            // this might not be the case if (e.g.) an edit was made by an action that
            // re-finalizes the model at t=0, so use the version number to track potential
            // situations where that might've happened (#932)
            if (m_Model->getState().getTime() != m_ScrubTime) {
                setScrubTime(m_ScrubTime);
            }
        }

        bool isModelLoaded() const
        {
            return HasInputFileName(m_Model->getModel());
        }

        void loadModelFile(const std::filesystem::path& p)
        {
            m_Model->loadModel(p);
            reinitializeModelFromBackingData("loaded model");
        }

        void reloadAll(std::string_view label = "reloaded model")
        {
            // reload/reset model
            if (HasInputFileName(m_Model->getModel())) {
                SceneCache dummy;
                ActionReloadOsimFromDisk(*m_Model, dummy);
            }
            else {
                m_Model->resetModel();
            }

            // if applicable, reload associated trajectory
            if (m_AssociatedTrajectory) {
                m_AssociatedTrajectory->reloadFromDisk(m_Model->getModel());
            }

            // reinitialize everything else
            reinitializeModelFromBackingData(label);
        }

        void loadModelTrajectoryFile(const std::filesystem::path& path)
        {
            m_AssociatedTrajectory = FileBackedStorage{m_Model->getModel(), path};
            reloadAll("loaded trajactory");
        }

        void loadMotionFiles(std::span<const std::filesystem::path> paths)
        {
            if (paths.empty()) {
                return;
            }

            m_AssociatedMotionFiles.insert(m_AssociatedMotionFiles.end(), paths.begin(), paths.end());
            reloadAll(paths.size() == 1 ? "loaded motion" : "loaded motions");
        }

        void loadXMLAsOpenSimDocument(std::span<const std::filesystem::path> paths)
        {
            if (paths.empty()) {
                return;
            }

            m_AssociatedXMLDocuments.insert(m_AssociatedXMLDocuments.end(), paths.begin(), paths.end());
            reloadAll(paths.size() == 1 ? "loaded XML document" : "loaded XML documents");
        }

        ClosedInterval<float> getTimeRange() const
        {
            return m_TimeRange;
        }

        void setTimeRange(ClosedInterval<float> newTimeRange)
        {
            m_TimeRange = newTimeRange;
        }

        double getScrubTime() const
        {
            return m_ScrubTime;
        }

        void setScrubTime(double newTime)
        {
            SimTK::State& state = m_Model->updModel().updWorkingState();
            state.setTime(newTime);

            if (m_AssociatedTrajectory) {
                UpdateStateFromStorageTime(
                    m_Model->updModel(),
                    state,
                    m_AssociatedTrajectory->mapper(),
                    m_AssociatedTrajectory->storage(),
                    newTime
                );
                // m_Model->updModel().assemble(state);
                // m_Model->updModel().equilibrateMuscles(state);
                m_Model->getModel().realizeReport(state);
            }
            else {
                // no associated motion: only change the time part of the state and re-realize
                m_Model->updModel().equilibrateMuscles(state);
                m_Model->updModel().realizeDynamics(state);
            }
            m_ScrubTime = static_cast<float>(newTime);
        }

        void rollbackModel()
        {
            m_Model->rollback();
        }
    private:
        void reinitializeModelFromBackingData(std::string_view label)
        {
            // hide forces that are computed from the model, because it's assumed that the
            // user only wants to visualize forces that come from externally-supplied data
            if (m_Model->getModel().countNumComponents() > 0) {
                for (auto& force : m_Model->updModel().updComponentList<OpenSim::Force>()) {
                    force.set_appliesForce(false);
                }
            }

            // (re)load associated trajectory
            if (m_AssociatedTrajectory) {
                InitializeModel(m_Model->updModel());
                m_AssociatedTrajectory->reloadFromDisk(m_Model->getModel());
            }

            // (re)load motions
            for (const std::filesystem::path& path : m_AssociatedMotionFiles) {
                m_Model->updModel().addModelComponent(std::make_unique<AnnotatedMotion>(path).release());
            }

            // (re)load associated XML files (e.g. `ExternalLoads`)
            for (const std::filesystem::path& path : m_AssociatedXMLDocuments) {
                auto ptr = std::unique_ptr<OpenSim::Object>{OpenSim::Object::makeObjectFromFile(path.string())};
                if (dynamic_cast<OpenSim::ModelComponent*>(ptr.get())) {
                    m_Model->updModel().addModelComponent(dynamic_cast<OpenSim::ModelComponent*>(ptr.release()));
                }
            }

            // care: state initialization is dependent on `m_AssociatedTrajectory`
            InitializeModel(m_Model->updModel());
            InitializeState(m_Model->updModel());
            m_Model->commit(label);
            setScrubTime(m_ScrubTime);
        }

        std::shared_ptr<UndoableModelStatePair> m_Model = std::make_shared<UndoableModelStatePair>();
        std::optional<FileBackedStorage> m_AssociatedTrajectory;
        std::vector<std::filesystem::path> m_AssociatedMotionFiles;
        std::vector<std::filesystem::path> m_AssociatedXMLDocuments;
        ClosedInterval<float> m_TimeRange = {0.0f, 10.0f};
        float m_ScrubTime = 0.0f;
    };

    class PreviewExperimentalDataTabToolbar final {
    public:
        explicit PreviewExperimentalDataTabToolbar(std::shared_ptr<PreviewExperimentalDataUiState> uiState) :
            m_UiState{std::move(uiState)}
        {}

        void onDraw()
        {
            if (BeginToolbar("##PreviewExperimentalDataToolbar", Vec2{5.0f, 5.0f})) {
                // load/reload etc.
                {
                    if (ui::draw_button("load model")) {
                        if (const auto path = prompt_user_to_select_file({"osim"})) {
                            m_UiState->loadModelFile(*path);
                        }
                    }

                    ui::same_line();
                    if (not m_UiState->isModelLoaded()) {
                        ui::begin_disabled();
                    }
                    if (ui::draw_button("load model trajectory/states")) {
                        if (const auto path = prompt_user_to_select_file({"sto", "mot"})) {
                            m_UiState->loadModelTrajectoryFile(*path);
                        }
                    }
                    if (not m_UiState->isModelLoaded()) {
                        ui::end_disabled();
                    }

                    ui::same_line();
                    if (ui::draw_button("load raw data file")) {
                        m_UiState->loadMotionFiles(prompt_user_to_select_files({"sto", "mot", "trc"}));
                    }

                    if (not m_UiState->isModelLoaded()) {
                        ui::begin_disabled();
                    }
                    ui::same_line();
                    if (ui::draw_button("load OpenSim XML")) {
                        m_UiState->loadXMLAsOpenSimDocument(prompt_user_to_select_files({"xml"}));
                    }
                    if (not m_UiState->isModelLoaded()) {
                        ui::end_disabled();
                    }

                    ui::same_line();
                    if (ui::draw_button(OSC_ICON_RECYCLE " reload all")) {
                        m_UiState->reloadAll();
                    }
                }

                // scrubber
                ui::draw_same_line_with_vertical_separator();
                {
                    ClosedInterval<float> tr = m_UiState->getTimeRange();
                    ui::set_next_item_width(ui::calc_text_size("<= xxxx").x);
                    if (ui::draw_float_input("<=", &tr.lower)) {
                        m_UiState->setTimeRange(tr);
                    }

                    ui::same_line();
                    auto t = static_cast<float>(m_UiState->getScrubTime());
                    ui::set_next_item_width(ui::calc_text_size("----------------------------------------------------------------").x);
                    if (ui::draw_float_slider("t", &t, static_cast<float>(tr.lower), static_cast<float>(tr.upper), "%.6f")) {
                        m_UiState->setScrubTime(t);
                    }

                    ui::same_line();
                    ui::draw_text("<=");
                    ui::same_line();
                    ui::set_next_item_width(ui::calc_text_size("<= xxxx").x);
                    if (ui::draw_float_input("##<=", &tr.upper)) {
                        m_UiState->setTimeRange(tr);
                    }
                }

                // scaling, visualization toggles.
                ui::draw_same_line_with_vertical_separator();
                {
                    DrawSceneScaleFactorEditorControls(m_UiState->updModel());
                    if (not m_IconCache) {
                        m_IconCache = App::singleton<IconCache>(
                            App::resource_loader().with_prefix("icons/"),
                            ui::get_text_line_height()/128.0f
                        );
                    }
                    ui::same_line();
                    DrawAllDecorationToggleButtons(m_UiState->updModel(), *m_IconCache);
                }


                ui::draw_same_line_with_vertical_separator();
            }
            ui::end_panel();
        }
    private:
        std::shared_ptr<PreviewExperimentalDataUiState> m_UiState;
        std::shared_ptr<IconCache> m_IconCache;
    };
}

class osc::PreviewExperimentalDataTab::Impl final : public TabPrivate {
public:
    explicit Impl(
        PreviewExperimentalDataTab& owner,
        Widget& parent) :
        TabPrivate{owner, &parent, OSC_ICON_DOT_CIRCLE " Experimental Data"}
    {
        m_PanelManager->register_toggleable_panel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_UiState->updSharedModelPtr(),
                    [this](const OpenSim::ComponentPath& p)
                    {
                        auto popup = std::make_unique<ComponentContextMenu>("##componentcontextmenu", this->owner(), m_UiState->updSharedModelPtr(), p);
                        App::post_event<OpenPopupEvent>(this->owner(), std::move(popup));
                    }
                );
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this->owner(), m_UiState->updSharedModelPtr());
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Coordinates",
            [this](std::string_view panelName)
            {
                return std::make_shared<CoordinateEditorPanel>(panelName, this->owner(),  m_UiState->updSharedModelPtr());
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Output Watches",
            [this](std::string_view panelName)
            {
                return std::make_shared<OutputWatchesPanel>(panelName, m_UiState->updSharedModelPtr());
            }
        );
        m_PanelManager->register_spawnable_panel(
            "viewer",
            [this](std::string_view panelName)
            {
                auto onRightClick = [model = m_UiState->updSharedModelPtr(), menuName = std::string{panelName} + "_contextmenu", editorAPI = this](const ModelViewerPanelRightClickEvent& e)
                {
                    auto popup = std::make_unique<ComponentContextMenu>(
                        menuName,
                        editorAPI->owner(),
                        model,
                        e.componentAbsPathOrEmpty
                    );

                    App::post_event<OpenPopupEvent>(editorAPI->owner(), std::move(popup));
                };
                ModelViewerPanelParameters panelParams{m_UiState->updSharedModelPtr(), onRightClick};

                return std::make_shared<ModelViewerPanel>(panelName, panelParams);
            },
            1  // have one viewer open at the start
        );
    }

    void on_mount()
    {
        m_PanelManager->on_mount();
        m_PopupManager.on_mount();
    }

    void on_unmount()
    {
        // m_PopupManager.on_unmount();
        m_PanelManager->on_unmount();
    }

    bool on_event(Event& e)
    {
        if (auto* openPopupEvent = dynamic_cast<OpenPopupEvent*>(&e)) {
            if (openPopupEvent->has_tab()) {
                auto tab = openPopupEvent->take_tab();
                tab->open();
                m_PopupManager.push_back(std::move(tab));
                return true;
            }
        }
        else if (auto* namedPanel = dynamic_cast<OpenNamedPanelEvent*>(&e)) {
            m_PanelManager->set_toggleable_panel_activated(namedPanel->panel_name(), true);
            return true;
        }
        else if (auto* contextMenuEvent = dynamic_cast<OpenComponentContextMenuEvent*>(&e)) {
            auto popup = std::make_unique<ComponentContextMenu>(
                "##componentcontextmenu",
                this->owner(),
                m_UiState->updSharedModelPtr(),
                contextMenuEvent->path()
            );
            App::post_event<OpenPopupEvent>(owner(), std::move(popup));
            return true;
        }
        return false;
    }

    void on_tick()
    {
        m_UiState->on_tick();
        m_PanelManager->on_tick();
    }

    void on_draw_main_menu()
    {
        m_WindowMenu.on_draw();
    }

    void on_draw()
    {
        try {
            ui::enable_dockspace_over_main_viewport();
            m_Toolbar.onDraw();
            m_PanelManager->on_draw();
            m_StatusBar.onDraw();
            m_PopupManager.on_draw();
            m_ThrewExceptionLastFrame = false;
        }
        catch (const std::exception& ex) {
            if (m_ThrewExceptionLastFrame) {
                throw;
            }

            m_ThrewExceptionLastFrame = true;
            log_error("error detected: %s", ex.what());
            log_error("rolling back model");
            m_UiState->rollbackModel();
        }
    }

private:
    std::shared_ptr<PreviewExperimentalDataUiState> m_UiState = std::make_shared<PreviewExperimentalDataUiState>();
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();
    PreviewExperimentalDataTabToolbar m_Toolbar{m_UiState};
    WindowMenu m_WindowMenu{m_PanelManager};
    ModelStatusBar m_StatusBar{*parent(), m_UiState->updSharedModelPtr()};
    PopupManager m_PopupManager;
    bool m_ThrewExceptionLastFrame = false;
};


CStringView osc::PreviewExperimentalDataTab::id() { return "OpenSim/Experimental/PreviewExperimentalData"; }

osc::PreviewExperimentalDataTab::PreviewExperimentalDataTab(Widget& ptr) :
    Tab{std::make_unique<Impl>(*this, ptr)}
{}
void osc::PreviewExperimentalDataTab::impl_on_mount() { private_data().on_mount(); }
void osc::PreviewExperimentalDataTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::PreviewExperimentalDataTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::PreviewExperimentalDataTab::impl_on_tick() { private_data().on_tick(); }
void osc::PreviewExperimentalDataTab::impl_on_draw_main_menu() { return private_data().on_draw_main_menu(); }
void osc::PreviewExperimentalDataTab::impl_on_draw() { private_data().on_draw(); }

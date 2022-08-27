#include "ModelEditorTab.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/MiddlewareAPIs/EditorAPI.hpp"
#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Tabs/ErrorTab.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/FileChangePoller.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"
#include "src/Widgets/BasicWidgets.hpp"
#include "src/Widgets/ComponentContextMenu.hpp"
#include "src/Widgets/CoordinateEditor.hpp"
#include "src/Widgets/EditorTabStatusBar.hpp"
#include "src/Widgets/LogViewer.hpp"
#include "src/Widgets/MainMenu.hpp"
#include "src/Widgets/ModelActionsMenuItems.hpp"
#include "src/Widgets/ModelHierarchyPanel.hpp"
#include "src/Widgets/ModelMusclePlotPanel.hpp"
#include "src/Widgets/OutputWatchesPanel.hpp"
#include "src/Widgets/ParamBlockEditorPopup.hpp"
#include "src/Widgets/PerfPanel.hpp"
#include "src/Widgets/Popup.hpp"
#include "src/Widgets/SelectionEditorPanel.hpp"
#include "src/Widgets/UiModelViewer.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <implot.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

static std::array<std::string, 7> const g_EditorScreenPanels =
{
    "Hierarchy",
    "Property Editor",
    "Log",
    "Coordinate Editor",
    "Performance",
    "Output Watches",
};

class osc::ModelEditorTab::Impl final : public EditorAPI {
public:
    Impl(MainUIStateAPI* parent, std::unique_ptr<UndoableModelStatePair> model) :
        m_Parent{std::move(parent)},
        m_Model{std::move(model)}
    {
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    bool isUnsaved() const
    {
        return !m_Model->isUpToDateWithFilesystem();
    }

    bool trySave()
    {
        return ActionSaveModel(m_Parent, *m_Model);
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_Name = computeTabName();
        ImPlot::CreateContext();
    }

    void onUnmount()
    {
        ImPlot::DestroyContext();
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_KEYDOWN)
        {
            return onKeydown(e.key);
        }
        else if (e.type == SDL_DROPFILE)
        {
            return onDropEvent(e.drop);
        }
        else
        {
            return false;
        }
    }

    void onTick()
    {
        if (m_FileChangePoller.changeWasDetected(m_Model->getModel().getInputFileName()))
        {
            osc::ActionUpdateModelFromBackingFile(*m_Model);
        }

        m_Name = computeTabName();
    }

    void onDrawMainMenu()
    {
        m_MainMenuFileTab.draw(m_Parent, m_Model.get());
        drawMainMenuEditTab();
        drawMainMenuSimulateTab();
        drawMainMenuWindowTab();
        m_MainMenuAboutTab.draw();

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
        if (ImGui::Button(ICON_FA_PLAY " Simulate (Ctrl+R)"))
        {
            osc::ActionStartSimulatingModel(m_Parent, *m_Model);
        }
        ImGui::PopStyleColor();

        if (ImGui::Button(ICON_FA_EDIT " Edit simulation settings"))
        {
            pushPopup(std::make_unique<ParamBlockEditorPopup>("simulation parameters", &m_Parent->updSimulationParams()));
        }
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        try
        {
            drawUNGUARDED();
            m_ExceptionThrownLastFrame = false;
        }
        catch (std::exception const& ex)
        {
            log::error("an std::exception was thrown while drawing the editor");
            log::error("    message = %s", ex.what());
            log::error("exceptions typically happen when the model is damaged or made invalid by an edit (e.g. setting a property to an invalid value)");

            if (m_ExceptionThrownLastFrame)
            {
                UID tabID = m_Parent->addTab<ErrorTab>(m_Parent, ex);
                m_Parent->selectTab(tabID);
                m_Parent->closeTab(m_ID);
            }
            else
            {
                try
                {
                    m_Model->rollback();
                    log::error("model rollback succeeded");
                    m_ExceptionThrownLastFrame = true;
                }
                catch (std::exception const& ex2)
                {
                    UID tabID = m_Parent->addTab<ErrorTab>(m_Parent, ex2);
                    m_Parent->selectTab(tabID);
                    m_Parent->closeTab(m_ID);
                }
            }

            m_Parent->resetImgui();
        }
    }

private:

    std::string computeTabName()
    {
        std::stringstream ss;
        ss << ICON_FA_EDIT << " ";
        ss << GetRecommendedDocumentName(*m_Model);
        return std::move(ss).str();
    }

    bool onDropEvent(SDL_DropEvent const& e)
    {
        if (e.file != nullptr && CStrEndsWith(e.file, ".sto"))
        {
            return osc::ActionLoadSTOFileAgainstModel(m_Parent, *m_Model, e.file);
        }

        return false;
    }

    bool onKeydown(SDL_KeyboardEvent const& e)
    {
        if (osc::IsCtrlOrSuperDown())
        {
            if (e.keysym.mod & KMOD_SHIFT)
            {
                switch (e.keysym.sym) {
                case SDLK_z:  // Ctrl+Shift+Z : undo focused model
                    osc::ActionRedoCurrentlyEditedModel(*m_Model);
                    return true;
                }
                return false;
            }

            switch (e.keysym.sym) {
            case SDLK_z:  // Ctrl+Z: undo focused model
                osc::ActionUndoCurrentlyEditedModel(*m_Model);
                return true;
            case SDLK_r:
            {
                // Ctrl+R: start a new simulation from focused model
                return osc::ActionStartSimulatingModel(m_Parent, *m_Model);
            }
            case SDLK_a:  // Ctrl+A: clear selection
                osc::ActionClearSelectionFromEditedModel(*m_Model);
                return true;
            }

            return false;
        }

        switch (e.keysym.sym) {
        case SDLK_BACKSPACE:
        case SDLK_DELETE:  // BACKSPACE/DELETE: delete selection
            osc::ActionTryDeleteSelectionFromEditedModel(*m_Model);
            return true;
        }

        return false;
    }

    void drawAddMusclePlotMenu(OpenSim::Muscle const& m)
    {
        if (ImGui::BeginMenu("Add Muscle Plot vs:"))
        {
            for (OpenSim::Coordinate const& c : m_Model->getModel().getComponentList<OpenSim::Coordinate>())
            {
                if (ImGui::MenuItem(c.getName().c_str()))
                {
                    addMusclePlot(c, m);
                }
            }

            ImGui::EndMenu();
        }
    }

    int getNumMusclePlots() const
    {
        return static_cast<int>(m_ModelMusclePlots.size());
    }

    ModelMusclePlotPanel const& getMusclePlot(int idx) const
    {
        return m_ModelMusclePlots.at(idx);
    }

    ModelMusclePlotPanel& updMusclePlot(int idx)
    {
        return m_ModelMusclePlots.at(idx);
    }

    void addMusclePlot()
    {
        m_ModelMusclePlots.emplace_back(m_Model, std::string{"MusclePlot_"} + std::to_string(m_LatestMusclePlot++));
    }

    void addMusclePlot(OpenSim::Coordinate const& coord, OpenSim::Muscle const& muscle) override
    {
        m_ModelMusclePlots.emplace_back(m_Model, std::string{"MusclePlot_"} + std::to_string(m_LatestMusclePlot++), coord.getAbsolutePath(), muscle.getAbsolutePath());
    }

    void removeMusclePlot(int idx)
    {
        OSC_ASSERT(0 <= idx && idx < static_cast<int>(m_ModelMusclePlots.size()));
        m_ModelMusclePlots.erase(m_ModelMusclePlots.begin() + idx);
    }

    void drawMainMenuEditTab()
    {
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, m_Model->canUndo()))
            {
                osc::ActionUndoCurrentlyEditedModel(*m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, m_Model->canRedo()))
            {
                osc::ActionRedoCurrentlyEditedModel(*m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_EYE_SLASH " Clear Isolation", nullptr, false, m_Model->getIsolated()))
            {
                osc::ActionSetModelIsolationTo(*m_Model, nullptr);
            }
            DrawTooltipIfItemHovered("Clear Isolation", "Clear current isolation setting. This is effectively the opposite of 'Isolate'ing a component.");

            {
                float scaleFactor = m_Model->getFixupScaleFactor();
                if (ImGui::InputFloat("set scale factor", &scaleFactor))
                {
                    osc::ActionSetModelSceneScaleFactorTo(*m_Model, scaleFactor);
                }
            }

            if (ImGui::MenuItem(ICON_FA_EXPAND_ARROWS_ALT " autoscale scale factor"))
            {
                osc::ActionAutoscaleSceneScaleFactor(*m_Model);
            }
            DrawTooltipIfItemHovered("Autoscale Scale Factor", "Try to autoscale the model's scale factor based on the current dimensions of the model");

            if (ImGui::MenuItem(ICON_FA_ARROWS_ALT " toggle frames"))
            {
                osc::ActionToggleFrames(*m_Model);
            }
            DrawTooltipIfItemHovered("Toggle Frames", "Set the model's display properties to display physical frames");

            bool modelHasBackingFile = osc::HasInputFileName(m_Model->getModel());

            if (ImGui::MenuItem(ICON_FA_REDO " Reload osim", nullptr, false, modelHasBackingFile))
            {
                osc::ActionReloadOsimFromDisk(*m_Model);
            }
            DrawTooltipIfItemHovered("Reload osim file", "Attempts to reload the osim file from scratch. This can be useful if (e.g.) editing third-party files that OpenSim Creator doesn't automatically track.");

            if (ImGui::MenuItem(ICON_FA_CLIPBOARD " Copy .osim path to clipboard", nullptr, false, modelHasBackingFile))
            {
                osc::ActionCopyModelPathToClipboard(*m_Model);
            }
            DrawTooltipIfItemHovered("Copy .osim path to clipboard", "Copies the absolute path to the model's .osim file into your clipboard.\n\nThis is handy if you want to (e.g.) load the osim via a script, open it from the command line in an other app, etc.");

            if (ImGui::MenuItem(ICON_FA_FOLDER " Open .osim's parent directory", nullptr, false, modelHasBackingFile))
            {
                osc::ActionOpenOsimParentDirectory(*m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_LINK " Open .osim in external editor", nullptr, false, modelHasBackingFile))
            {
                osc::ActionOpenOsimInExternalEditor(*m_Model);
            }
            DrawTooltipIfItemHovered("Open .osim in external editor", "Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");

            ImGui::EndMenu();
        }
    }

    void drawMainMenuSimulateTab()
    {
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem(ICON_FA_PLAY " Simulate", "Ctrl+R"))
            {
                osc::ActionStartSimulatingModel(m_Parent, *m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_EDIT " Edit simulation settings"))
            {
                pushPopup(std::make_unique<ParamBlockEditorPopup>("simulation parameters", &m_Parent->updSimulationParams()));
            }

            if (ImGui::MenuItem("Disable all wrapping surfaces"))
            {
                osc::ActionDisableAllWrappingSurfaces(*m_Model);
            }

            if (ImGui::MenuItem("Enable all wrapping surfaces"))
            {
                osc::ActionEnableAllWrappingSurfaces(*m_Model);
            }

            if (ImGui::MenuItem("Simulate Against All Integrators (advanced)"))
            {
                osc::ActionSimulateAgainstAllIntegrators(m_Parent, *m_Model);
            }
            osc::DrawTooltipIfItemHovered("Simulate Against All Integrators", "Simulate the given model against all available SimTK integrators. This takes the current simulation parameters and permutes the integrator, reporting the overall simulation wall-time to the user. It's an advanced feature that's handy for developers to figure out which integrator best-suits a particular model");

            ImGui::EndMenu();
        }
    }

    void drawMainMenuWindowTab()
    {
        // draw "window" tab
        if (ImGui::BeginMenu("Window"))
        {
            Config const& cfg = App::get().getConfig();
            for (std::string const& panel : g_EditorScreenPanels)
            {
                bool currentVal = cfg.getIsPanelEnabled(panel);
                if (ImGui::MenuItem(panel.c_str(), nullptr, &currentVal))
                {
                    App::upd().updConfig().setIsPanelEnabled(panel, currentVal);
                }
            }

            ImGui::Separator();

            // active 3D viewers (can be disabled)
            for (int i = 0; i < static_cast<int>(m_ModelViewers.size()); ++i)
            {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "viewer%i", i);

                bool enabled = true;
                if (ImGui::MenuItem(buf, nullptr, &enabled))
                {
                    m_ModelViewers.erase(m_ModelViewers.begin() + i);
                    --i;
                }
            }

            if (ImGui::MenuItem("add viewer"))
            {
                m_ModelViewers.emplace_back();
            }

            ImGui::Separator();

            // active muscle plots
            for (int i = 0; i < getNumMusclePlots(); ++i)
            {
                ModelMusclePlotPanel const& plot = getMusclePlot(i);

                bool enabled = true;
                if (!plot.isOpen() || ImGui::MenuItem(plot.getName().c_str(), nullptr, &enabled))
                {
                    removeMusclePlot(i);
                    --i;
                }
            }

            if (ImGui::MenuItem("add muscle plot"))
            {
                addMusclePlot();
            }

            ImGui::EndMenu();
        }
    }

    // draw a single 3D model viewer
    bool draw3DViewer(osc::UiModelViewer& viewer, char const* name)
    {
        bool isOpen = true;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        bool shown = ImGui::Begin(name, &isOpen, ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();

        if (!isOpen)
        {
            ImGui::End();
            return false;  // closed by the user
        }

        if (!shown)
        {
            ImGui::End();
            return true;  // it's open, but not shown
        }

        auto resp = viewer.draw(*m_Model);
        ImGui::End();

        // update hover
        if (resp.isMousedOver && resp.hovertestResult != m_Model->getHovered())
        {
            m_Model->setHovered(resp.hovertestResult);
        }

        // if left-clicked, update selection
        if (viewer.isLeftClicked() && resp.isMousedOver)
        {
            m_Model->setSelected(resp.hovertestResult);
        }

        // if hovered, draw hover tooltip
        if (resp.isMousedOver && resp.hovertestResult)
        {
            DrawComponentHoverTooltip(*resp.hovertestResult);
        }

        // if right-clicked, draw context menu
        if (viewer.isRightClicked() && resp.isMousedOver)
        {
            std::string menuName = std::string{name} + "_contextmenu";
            OpenSim::ComponentPath path = resp.hovertestResult ? resp.hovertestResult->getAbsolutePath() : OpenSim::ComponentPath{};
            pushPopup(std::make_unique<ComponentContextMenu>(menuName, m_Parent, this, m_Model, path));
        }

        return true;
    }

    // draw all user-enabled 3D model viewers
    void draw3DViewers()
    {
        for (int i = 0; i < static_cast<int>(m_ModelViewers.size()); ++i)
        {
            osc::UiModelViewer& viewer = m_ModelViewers[i];

            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%i", i);

            bool isOpen = draw3DViewer(viewer, buf);

            if (!isOpen)
            {
                m_ModelViewers.erase(m_ModelViewers.begin() + i);
                --i;
            }
        }
    }

    void drawUNGUARDED()
    {
        // draw 3D viewers (if any)
        {
            OSC_PERF("draw 3D viewer(s)");
            draw3DViewers();
        }

        // draw editor actions panel
        //
        // contains top-level actions (e.g. "add body")
        osc::Config const& config = osc::App::get().getConfig();

        // draw hierarchy viewer
        {
            OSC_PERF("draw component hierarchy");

            auto resp = m_ComponentHierarchyPanel.draw(*m_Model);

            if (resp.type == osc::ModelHierarchyPanel::ResponseType::SelectionChanged)
            {
                m_Model->setSelected(resp.ptr);
            }
            else if (resp.type == osc::ModelHierarchyPanel::ResponseType::HoverChanged)
            {
                m_Model->setHovered(resp.ptr);
            }
        }

        // draw property editor
        if (bool propertyEditorOldState = config.getIsPanelEnabled("Property Editor"))
        {
            OSC_PERF("draw property editor");

            bool propertyEditorState = propertyEditorOldState;
            if (ImGui::Begin("Property Editor", &propertyEditorState))
            {
                m_SelectionEditor.draw();
            }
            ImGui::End();

            if (propertyEditorState != propertyEditorOldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Property Editor", propertyEditorState);
            }
        }

        // draw application log
        if (bool logOldState = config.getIsPanelEnabled("Log"))
        {
            OSC_PERF("draw log");

            bool logState = logOldState;
            if (ImGui::Begin("Log", &logState, ImGuiWindowFlags_MenuBar))
            {
                m_LogViewer.draw();
            }
            ImGui::End();

            if (logState != logOldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Log", logState);
            }
        }

        // draw coordinate editor
        if (bool coordEdOldState = config.getIsPanelEnabled("Coordinate Editor"))
        {
            OSC_PERF("draw coordinate editor");

            bool coordEdState = coordEdOldState;
            if (ImGui::Begin("Coordinate Editor", &coordEdState))
            {
                m_CoordEditor.draw();
            }
            ImGui::End();

            if (coordEdState != coordEdOldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Coordinate Editor", coordEdState);
            }
        }

        if (bool oldState = config.getIsPanelEnabled("Output Watches"))
        {
            OSC_PERF("draw output watches panel");

            m_OutputWatchesPanel.open();
            bool newState = m_OutputWatchesPanel.draw();
            if (newState != oldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Output Watches", newState);
            }
        }

        // draw performance viewer
        if (bool perfOldState = config.getIsPanelEnabled("Performance"))
        {
            OSC_PERF("draw performance panel");

            m_PerfPanel.open();
            bool state = m_PerfPanel.draw();

            if (state != perfOldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Performance", state);
            }
        }

        {
            OSC_PERF("draw muscle plots");

            // draw model muscle plots (if applicable)
            for (int i = 0; i < getNumMusclePlots(); ++i)
            {
                updMusclePlot(i).draw();
            }
        }

        // draw bottom status bar
        m_StatusBar.draw();

        // draw any generic popups pushed to this layer
        {
            // begin and (if applicable) draw bottom-to-top in a nested fashion
            int nOpened = 0;
            int nPopups = static_cast<int>(m_Popups.size());  // only draw the popups that existed at the start of this frame, not the ones added during this frame
            for (int i = 0; i < nPopups; ++i)
            {
                if (m_Popups[i]->beginPopup())
                {
                    m_Popups[i]->drawPopupContent();
                    ++nOpened;
                }
                else
                {
                    break;
                }
            }

            // end the opened popups top-to-bottom
            for (int i = nOpened-1; i >= 0; --i)
            {
                m_Popups[i]->endPopup();
            }

            // garbage-collect any closed popups
            osc::RemoveErase(m_Popups, [](auto const& ptr) { return !ptr->isOpen(); });
        }
    }

    void pushPopup(std::unique_ptr<Popup> popup) override
    {
        popup->open();
        m_Popups.push_back(std::move(popup));
    }

    UID m_ID;
    std::string m_Name = "ModelEditorTab";
    MainUIStateAPI* m_Parent;
    std::shared_ptr<UndoableModelStatePair> m_Model;

    // polls changes to a file
    FileChangePoller m_FileChangePoller{std::chrono::milliseconds{1000}, m_Model->getModel().getInputFileName()};

    // UI widgets/popups
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    LogViewer m_LogViewer;
    ModelHierarchyPanel m_ComponentHierarchyPanel{"Hierarchy"};
    CoordinateEditor m_CoordEditor{m_Model};
    PerfPanel m_PerfPanel{"Performance"};
    OutputWatchesPanel m_OutputWatchesPanel{"Output Watches", m_Model, m_Parent};
    SelectionEditorPanel m_SelectionEditor{m_Model};
    int m_LatestMusclePlot = 1;
    std::vector<ModelMusclePlotPanel> m_ModelMusclePlots;
    EditorTabStatusBar m_StatusBar{m_Parent, this, m_Model};
    std::vector<UiModelViewer> m_ModelViewers = std::vector<UiModelViewer>(1);
    std::vector<std::unique_ptr<Popup>> m_Popups;

    // flag that's set+reset each frame to prevent continual
    // throwing
    bool m_ExceptionThrownLastFrame = false;
};


// public API

osc::ModelEditorTab::ModelEditorTab(MainUIStateAPI* parent,  std::unique_ptr<UndoableModelStatePair> model) :
    m_Impl{new Impl{std::move(parent), std::move(model)}}
{
}

osc::ModelEditorTab::ModelEditorTab(ModelEditorTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ModelEditorTab& osc::ModelEditorTab::operator=(ModelEditorTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ModelEditorTab::~ModelEditorTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::ModelEditorTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ModelEditorTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::ModelEditorTab::implParent() const
{
    return m_Impl->parent();
}

bool osc::ModelEditorTab::implIsUnsaved() const
{
    return m_Impl->isUnsaved();
}

bool osc::ModelEditorTab::implTrySave()
{
    return m_Impl->trySave();
}

void osc::ModelEditorTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ModelEditorTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ModelEditorTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ModelEditorTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ModelEditorTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ModelEditorTab::implOnDraw()
{
    m_Impl->onDraw();
}

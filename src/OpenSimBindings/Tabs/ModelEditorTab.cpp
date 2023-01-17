#include "ModelEditorTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/Tabs/LoadingTab.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/ComponentContextMenu.hpp"
#include "src/OpenSimBindings/Widgets/CoordinateEditorPanel.hpp"
#include "src/OpenSimBindings/Widgets/EditorTabStatusBar.hpp"
#include "src/OpenSimBindings/Widgets/NavigatorPanel.hpp"
#include "src/OpenSimBindings/Widgets/ModelEditorMainMenu.hpp"
#include "src/OpenSimBindings/Widgets/ModelMusclePlotPanel.hpp"
#include "src/OpenSimBindings/Widgets/ModelEditorToolbar.hpp"
#include "src/OpenSimBindings/Widgets/OutputWatchesPanel.hpp"
#include "src/OpenSimBindings/Widgets/ParamBlockEditorPopup.hpp"
#include "src/OpenSimBindings/Widgets/PropertiesPanel.hpp"
#include "src/OpenSimBindings/Widgets/UiModelViewer.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Tabs/ErrorTab.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/FileChangePoller.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"
#include "src/Widgets/LogViewerPanel.hpp"
#include "src/Widgets/PerfPanel.hpp"
#include "src/Widgets/Panel.hpp"
#include "src/Widgets/Popup.hpp"
#include "src/Widgets/Popups.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <implot.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>

#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

class osc::ModelEditorTab::Impl final : public EditorAPI {
public:

    Impl(MainUIStateAPI* parent,
        std::unique_ptr<UndoableModelStatePair> model) :

        m_ParentAPI{std::move(parent)},
        m_Model{std::move(model)}
    {
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return m_TabName;
    }

    TabHost* parent()
    {
        return m_ParentAPI;
    }

    bool isUnsaved() const
    {
        return !m_Model->isUpToDateWithFilesystem();
    }

    bool trySave()
    {
        return ActionSaveModel(*m_ParentAPI, *m_Model);
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_TabName = computeTabName();
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
            return onKeydownEvent(e.key);
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

        m_TabName = computeTabName();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.draw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );

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
                UID tabID = m_ParentAPI->addTab<ErrorTab>(m_ParentAPI, ex);
                m_ParentAPI->selectTab(tabID);
                m_ParentAPI->closeTab(m_TabID);
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
                    log::error("model rollback also thrown an exception: %s", ex2.what());
                    UID tabID = m_ParentAPI->addTab<ErrorTab>(m_ParentAPI, ex2);
                    m_ParentAPI->selectTab(tabID);
                    m_ParentAPI->closeTab(m_TabID);
                }
            }

            m_ParentAPI->resetImgui();
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
            return osc::ActionLoadSTOFileAgainstModel(*m_ParentAPI, *m_Model, e.file);
        }
        else if (e.type == SDL_DROPFILE && e.file != nullptr && CStrEndsWith(e.file, ".osim"))
        {
            // if the user drops an osim file on this tab then it should be loaded
            UID const tabID = m_ParentAPI->addTab<LoadingTab>(m_ParentAPI, e.file);
            m_ParentAPI->selectTab(tabID);
            return true;
        }

        return false;
    }

    bool onKeydownEvent(SDL_KeyboardEvent const& e)
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
                return osc::ActionStartSimulatingModel(*m_ParentAPI, *m_Model);
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

    // draw a single 3D model viewer
    bool draw3DViewer(osc::UiModelViewer& viewer, char const* name)
    {
        bool isOpen = true;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        bool shown = ImGui::Begin(name, &isOpen);
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
            pushPopup(std::make_unique<ComponentContextMenu>(menuName, m_ParentAPI, this, m_Model, path));
        }

        return true;
    }

    // draw all user-enabled 3D model viewers
    void draw3DViewers()
    {
        for (int i = 0; i < static_cast<int>(m_ModelViewers.size()); ++i)
        {
            osc::UiModelViewer& viewer = m_ModelViewers[i];

            std::string const name = getModelVisualizerName(i);

            bool isOpen = draw3DViewer(viewer, name.c_str());

            if (!isOpen)
            {
                m_ModelViewers.erase(m_ModelViewers.begin() + i);
                --i;
            }
        }
    }

    void drawUNGUARDED()
    {
        m_Toolbar.draw();

        // draw 3D viewers (if any)
        {
            OSC_PERF("draw 3D viewer(s)");
            draw3DViewers();
        }

        // draw navigator
        {
            OSC_PERF("draw navigator panel");

            auto resp = m_NavigatorPanel.draw(*m_Model);

            if (resp.type == osc::NavigatorPanel::ResponseType::SelectionChanged)
            {
                m_Model->setSelected(resp.ptr);
            }
            else if (resp.type == osc::NavigatorPanel::ResponseType::HoverChanged)
            {
                m_Model->setHovered(resp.ptr);
            }
        }

        m_PropertiesPanel.draw();
        m_LogViewerPanel.draw();
        m_CoordinatesPanel.draw();
        m_OutputWatchesPanel.draw();
        m_PerformancePanel.draw();

        {
            OSC_PERF("draw muscle plots");

            // draw model muscle plots (if applicable)
            for (size_t i = 0; i < m_ModelMusclePlots.size(); ++i)
            {
                m_ModelMusclePlots.at(static_cast<size_t>(i)).draw();
            }
        }

        m_StatusBar.draw();

        // draw any generic popups pushed to this layer
        m_Popups.draw();
    }

    // EditorAPI IMPL

    void implPushComponentContextMenuPopup(OpenSim::ComponentPath const& path) final
    {
        auto popup = std::make_unique<ComponentContextMenu>(
            "##componentcontextmenu",
            m_ParentAPI,
            this,
            m_Model,
            path
            );
        pushPopup(std::move(popup));
    }

    void implPushPopup(std::unique_ptr<Popup> popup) final
    {
        popup->open();
        m_Popups.push_back(std::move(popup));
    }

    size_t implGetNumMusclePlots() final
    {
        return m_ModelMusclePlots.size();
    }

    ModelMusclePlotPanel const& implGetMusclePlot(ptrdiff_t i) final
    {
        return m_ModelMusclePlots.at(i);
    }

    void implAddEmptyMusclePlot() final
    {
        m_ModelMusclePlots.emplace_back(this, m_Model, std::string{"MusclePlot_"} + std::to_string(m_LatestMusclePlot++));
    }

    void implAddMusclePlot(OpenSim::Coordinate const& coord, OpenSim::Muscle const& muscle) final
    {
        m_ModelMusclePlots.emplace_back(this, m_Model, std::string{"MusclePlot_"} + std::to_string(m_LatestMusclePlot++), coord.getAbsolutePath(), muscle.getAbsolutePath());
    }

    void implDeleteMusclePlot(ptrdiff_t i) final
    {
        m_ModelMusclePlots.erase(m_ModelMusclePlots.begin() + i);
    }

    void implAddVisualizer() final
    {
        m_ModelViewers.emplace_back();
    }

    size_t implGetNumModelVisualizers() final
    {
        return m_ModelViewers.size();
    }

    std::string implGetModelVisualizerName(ptrdiff_t i) final
    {
        return "viewer" + std::to_string(i);
    }

    void implDeleteVisualizer(ptrdiff_t i) final
    {
        m_ModelViewers.erase(m_ModelViewers.begin() + i);
    }

    // tab top-level data
    UID m_TabID;
    std::string m_TabName = "ModelEditorTab";
    MainUIStateAPI* m_ParentAPI;

    // the model being edited
    std::shared_ptr<UndoableModelStatePair> m_Model;

    // polls changes to a file
    FileChangePoller m_FileChangePoller
    {
        std::chrono::milliseconds{1000},  // polling rate
        m_Model->getModel().getInputFileName(),
    };

    // static UI widgets/popups
    ModelEditorMainMenu m_MainMenu{m_ParentAPI, this, m_Model};
    ModelEditorToolbar m_Toolbar{"##ModelEditorToolbar", m_ParentAPI, this, m_Model};
    LogViewerPanel m_LogViewerPanel{"Log"};
    NavigatorPanel m_NavigatorPanel{"Navigator", [this](OpenSim::ComponentPath const& p)  { this->pushPopup(std::make_unique<ComponentContextMenu>("##componentcontextmenu", m_ParentAPI, this, m_Model, p)); }};
    CoordinateEditorPanel m_CoordinatesPanel{"Coordinates", m_ParentAPI, this, m_Model};
    PerfPanel m_PerformancePanel{"Performance"};
    OutputWatchesPanel m_OutputWatchesPanel{"Output Watches", m_Model, m_ParentAPI};
    PropertiesPanel m_PropertiesPanel{"Properties", this, m_Model};
    EditorTabStatusBar m_StatusBar{m_ParentAPI, this, m_Model};

    // dynamic UI widgets/popups
    int m_LatestMusclePlot = 1;
    std::vector<ModelMusclePlotPanel> m_ModelMusclePlots;
    std::vector<UiModelViewer> m_ModelViewers = std::vector<UiModelViewer>(1);
    Popups m_Popups;

    // flag that's set+reset each frame to prevent continual throwing
    bool m_ExceptionThrownLastFrame = false;
};


// public API (PIMPL)

osc::ModelEditorTab::ModelEditorTab(
    MainUIStateAPI* parent,
    std::unique_ptr<UndoableModelStatePair> model) :

    m_Impl{std::make_unique<Impl>(std::move(parent), std::move(model))}
{
}

osc::ModelEditorTab::ModelEditorTab(ModelEditorTab&&) noexcept = default;
osc::ModelEditorTab& osc::ModelEditorTab::operator=(ModelEditorTab&&) noexcept = default;
osc::ModelEditorTab::~ModelEditorTab() noexcept = default;

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

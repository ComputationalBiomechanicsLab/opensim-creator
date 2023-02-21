#include "ModelEditorTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/Panels/CoordinateEditorPanel.hpp"
#include "src/OpenSimBindings/Panels/NavigatorPanel.hpp"
#include "src/OpenSimBindings/Panels/ModelEditorViewerPanel.hpp"
#include "src/OpenSimBindings/Panels/ModelMusclePlotPanel.hpp"
#include "src/OpenSimBindings/Panels/OutputWatchesPanel.hpp"
#include "src/OpenSimBindings/Panels/PropertiesPanel.hpp"
#include "src/OpenSimBindings/Tabs/LoadingTab.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/ComponentContextMenu.hpp"
#include "src/OpenSimBindings/Widgets/EditorTabStatusBar.hpp"
#include "src/OpenSimBindings/Widgets/ModelEditorMainMenu.hpp"
#include "src/OpenSimBindings/Widgets/ModelEditorToolbar.hpp"
#include "src/OpenSimBindings/Widgets/ParamBlockEditorPopup.hpp"
#include "src/OpenSimBindings/Widgets/UiModelViewer.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Panels/LogViewerPanel.hpp"
#include "src/Panels/PerfPanel.hpp"
#include "src/Panels/Panel.hpp"
#include "src/Panels/PanelManager.hpp"
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
#include "src/Widgets/Popup.hpp"
#include "src/Widgets/Popups.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
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

    Impl(
        std::weak_ptr<MainUIStateAPI> parent_,
        std::unique_ptr<UndoableModelStatePair> model_) :

        m_Parent{std::move(parent_)},
        m_Model{std::move(model_)}
    {
        // register all panels that the editor tab supports

        m_PanelManager->registerToggleablePanel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_Model,
                    [this](OpenSim::ComponentPath const& p) 
                    {
                        pushPopup(std::make_unique<ComponentContextMenu>("##componentcontextmenu", m_Parent, this, m_Model, p));
                    }
                );
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this, m_Model);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Log",
            [this](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Coordinates",
            [this](std::string_view panelName)
            {
                return std::make_shared<CoordinateEditorPanel>(panelName, m_Parent, this, m_Model);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Output Watches",
            [this](std::string_view panelName)
            {
                return std::make_shared<OutputWatchesPanel>(panelName, m_Model, m_Parent);
            }
        );
        m_PanelManager->registerSpawnablePanel(
            "viewer",
            [this](std::string_view panelName)
            {
                return std::make_shared<ModelEditorViewerPanel>(panelName, m_Parent, this, m_Model);
            }
        );
        m_PanelManager->registerSpawnablePanel(
            "muscleplot",
            [this](std::string_view panelName)
            {
                return std::make_shared<ModelMusclePlotPanel>(this, m_Model, panelName);
            }
        );

        // push one viewer open at the start
        m_PanelManager->pushDynamicPanel(
            "viewer",
            std::make_shared<ModelEditorViewerPanel>(
                m_PanelManager->computeSuggestedDynamicPanelName("viewer"),
                m_Parent,
                this,
                m_Model
            )
        );
        m_PanelManager->activateAllDefaultOpenPanels();
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return m_TabName;
    }

    bool isUnsaved() const
    {
        return !m_Model->isUpToDateWithFilesystem();
    }

    bool trySave()
    {
        return ActionSaveModel(*m_Parent.lock(), *m_Model);
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_TabName = computeTabName();
    }

    void onUnmount()
    {
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
        m_PanelManager->garbageCollectDeactivatedPanels();
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
            m_Toolbar.draw();
            m_PanelManager->drawAllActivatedPanels();
            m_StatusBar.draw();
            m_Popups.draw();

            m_ExceptionThrownLastFrame = false;
        }
        catch (std::exception const& ex)
        {
            log::error("an std::exception was thrown while drawing the editor");
            log::error("    message = %s", ex.what());
            log::error("exceptions typically happen when the model is damaged or made invalid by an edit (e.g. setting a property to an invalid value)");

            if (m_ExceptionThrownLastFrame)
            {
                m_Parent.lock()->addAndSelectTab<ErrorTab>(m_Parent, ex);
                m_Parent.lock()->closeTab(m_TabID);
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
                    m_Parent.lock()->addAndSelectTab<ErrorTab>(m_Parent, ex2);
                    m_Parent.lock()->closeTab(m_TabID);
                }
            }

            m_Parent.lock()->resetImgui();
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
        else if (e.type == SDL_DROPFILE && e.file != nullptr && CStrEndsWith(e.file, ".osim"))
        {
            // if the user drops an osim file on this tab then it should be loaded
            m_Parent.lock()->addAndSelectTab<LoadingTab>(m_Parent, e.file);
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

    void implPushComponentContextMenuPopup(OpenSim::ComponentPath const& path) final
    {
        auto popup = std::make_unique<ComponentContextMenu>(
            "##componentcontextmenu",
            m_Parent,
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

    void implAddMusclePlot(OpenSim::Coordinate const& coord, OpenSim::Muscle const& muscle) final
    {
        std::string const name = m_PanelManager->computeSuggestedDynamicPanelName("muscleplot");
        m_PanelManager->pushDynamicPanel(
            "muscleplot",
            std::make_shared<ModelMusclePlotPanel>(this, m_Model, name, osc::GetAbsolutePath(coord), osc::GetAbsolutePath(muscle))
        );
    }

    std::shared_ptr<PanelManager> implGetPanelManager() final
    {
        return m_PanelManager;
    }

    // tab top-level data
    UID m_TabID;
    std::weak_ptr<MainUIStateAPI> m_Parent;
    std::string m_TabName = "ModelEditorTab";

    // the model being edited
    std::shared_ptr<UndoableModelStatePair> m_Model;

    // polls changes to a file
    FileChangePoller m_FileChangePoller
    {
        std::chrono::milliseconds{1000},  // polling rate
        m_Model->getModel().getInputFileName(),
    };

    // manager for toggleable and spawnable UI panels
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();

    // non-toggleable UI panels/menus/toolbars
    ModelEditorMainMenu m_MainMenu{m_Parent, this, m_Model};
    ModelEditorToolbar m_Toolbar{"##ModelEditorToolbar", m_Parent, this, m_Model};
    EditorTabStatusBar m_StatusBar{m_Parent, this, m_Model};

    // manager for popups that are open in this tab
    Popups m_Popups;

    // flag that's set+reset each frame to prevent continual throwing
    bool m_ExceptionThrownLastFrame = false;
};


// public API (PIMPL)

osc::ModelEditorTab::ModelEditorTab(
    std::weak_ptr<MainUIStateAPI> parent_,
    std::unique_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(std::move(parent_), std::move(model_))}
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

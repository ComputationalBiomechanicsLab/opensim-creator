#include "ModelEditorTab.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/LoadingTab.h>
#include <OpenSimCreator/UI/ModelEditor/ComponentContextMenu.h>
#include <OpenSimCreator/UI/ModelEditor/CoordinateEditorPanel.h>
#include <OpenSimCreator/UI/ModelEditor/EditorTabStatusBar.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorMainMenu.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorToolbar.h>
#include <OpenSimCreator/UI/ModelEditor/ModelMusclePlotPanel.h>
#include <OpenSimCreator/UI/ModelEditor/OutputWatchesPanel.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanel.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelParameters.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelRightClickEvent.h>
#include <OpenSimCreator/UI/Shared/NavigatorPanel.h>
#include <OpenSimCreator/UI/Shared/PropertiesPanel.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Tabs/ErrorTab.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/UI/Widgets/IPopup.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/FileChangePoller.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/Perf.h>
#include <oscar/Utils/UID.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>

#include <chrono>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

class osc::ModelEditorTab::Impl final : public IEditorAPI {
public:

    Impl(
        ParentPtr<IMainUIStateAPI> const& parent_,
        std::unique_ptr<UndoableModelStatePair> model_) :

        m_Parent{parent_},
        m_Model{std::move(model_)}
    {
        // register all panels that the editor tab supports

        m_PanelManager->register_toggleable_panel(
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
        m_PanelManager->register_toggleable_panel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this, m_Model);
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
                return std::make_shared<CoordinateEditorPanel>(panelName, m_Parent, this, m_Model);
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
                return std::make_shared<OutputWatchesPanel>(panelName, m_Model, m_Parent);
            }
        );
        m_PanelManager->register_spawnable_panel(
            "viewer",
            [this](std::string_view panelName)
            {
                auto onRightClick = [model = m_Model, menuName = std::string{panelName} + "_contextmenu", editorAPI = this, mainUIStateAPI = m_Parent](ModelEditorViewerPanelRightClickEvent const& e)
                {
                    editorAPI->pushPopup(std::make_unique<ComponentContextMenu>(
                        menuName,
                        mainUIStateAPI,
                        editorAPI,
                        model,
                        e.componentAbsPathOrEmpty
                    ));
                };
                ModelEditorViewerPanelParameters panelParams{m_Model, onRightClick};

                return std::make_shared<ModelEditorViewerPanel>(panelName, panelParams);
            },
            1  // have one viewer open at the start
        );
        m_PanelManager->register_spawnable_panel(
            "muscleplot",
            [this](std::string_view panelName)
            {
                return std::make_shared<ModelMusclePlotPanel>(this, m_Model, panelName);
            },
            0  // no muscle plots open at the start
        );
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
        return ActionSaveModel(*m_Parent, *m_Model);
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        m_TabName = computeTabName();
        m_PopupManager.on_mount();
        m_PanelManager->on_mount();
    }

    void on_unmount()
    {
        m_PanelManager->on_unmount();
        App::upd().make_main_loop_polling();
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

    void on_tick()
    {
        if (m_FileChangePoller.changeWasDetected(m_Model->getModel().getInputFileName()))
        {
            ActionUpdateModelFromBackingFile(*m_Model);
        }

        m_TabName = computeTabName();
        m_PanelManager->on_tick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ui::DockSpaceOverViewport(
            ui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );

        try
        {
            m_Toolbar.onDraw();
            m_PanelManager->on_draw();
            m_StatusBar.onDraw();
            m_PopupManager.onDraw();

            m_ExceptionThrownLastFrame = false;
        }
        catch (std::exception const& ex)
        {
            tryRecoveringFromException(ex);
        }
    }

    void tryRecoveringFromException(std::exception const& ex)
    {
        log_error("an std::exception was thrown while drawing the model editor");
        log_error("    message = %s", ex.what());
        log_error("exceptions typically happen when the model is damaged or made invalid by an edit (e.g. setting a property to an invalid value)");

        if (m_ExceptionThrownLastFrame)
        {
            if (m_Model->canUndo())
            {
                // exception was thrown last frame, indicating the model in the undo/redo buffer is also
                // damaged, so try undoing

                log_error("an exception was also thrown last frame, indicating model damage: attempting to undo to an earlier version of the model to try and fix the model");

                try
                {
                    m_Model->doUndo();  // TODO: add `doUndoWithNoRedoStorage` so that the user's redo buffer isn't tainted
                }
                catch (std::exception const& ex2)
                {
                    log_error("undoing the model also failed with error: %s", ex2.what());
                    log_error("because the model isn't recoverable, closing the editor tab");
                    m_Parent->add_and_select_tab<ErrorTab>(m_Parent, ex);
                    m_Parent->close_tab(m_TabID);  // TODO: should be forcibly closed with no "save" prompt
                }

                log_error("sucessfully undone model");
                m_ExceptionThrownLastFrame = false;  // reset flag
            }
            else if (!m_PopupManager.empty())
            {
                // exception was thrown last frame, but we can't undo the model, so try to assume that a popup was
                // causing the problem last frame and clear all popups instead of fully exploding the whole tab
                log_error("trying to close all currently-open popups, in case that prevents crashes");
                m_PopupManager.clear();
            }
            else
            {
                // exception thrown last frame, indicating the model in the undo/redo buffer is also damaged,
                // but cannot undo, so quit

                log_error("because the model isn't recoverable, closing the editor tab");
                m_Parent->add_and_select_tab<ErrorTab>(m_Parent, ex);
                m_Parent->close_tab(m_TabID);  // TODO: should be forcibly closed
            }
        }
        else
        {
            // no exception last frame, indicating the _scratch space_ may be damaged, so try to rollback
            // to a version in the undo/redo buffer

            try
            {
                log_error("attempting to rollback the model edit to a clean state");
                m_Model->rollback();
                log_error("model rollback succeeded");
                m_ExceptionThrownLastFrame = true;
            }
            catch (std::exception const& ex2)
            {
                log_error("model rollback thrown an exception: %s", ex2.what());
                log_error("because the model cannot be rolled back, closing the editor tab");
                m_Parent->add_and_select_tab<ErrorTab>(m_Parent, ex2);
                m_Parent->close_tab(m_TabID);
            }
        }

        // reset ImGui, because the exception unroll may have damaged ImGui state
        m_Parent->reset_imgui();
    }

private:

    std::string computeTabName()
    {
        std::stringstream ss;
        ss << ICON_FA_EDIT << " ";
        ss << m_Model->recommendedDocumentName();
        return std::move(ss).str();
    }

    bool onDropEvent(SDL_DropEvent const& e)
    {
        if (e.type != SDL_DROPFILE || e.file == nullptr)
        {
            return false;
        }
        std::string_view const filename{e.file};

        if (filename.ends_with(".sto"))
        {
            return ActionLoadSTOFileAgainstModel(m_Parent, *m_Model, e.file);
        }
        else if (filename.ends_with(".osim"))
        {
            // if the user drops an osim file on this tab then it should be loaded
            m_Parent->add_and_select_tab<LoadingTab>(m_Parent, e.file);
            return true;
        }

        return false;
    }

    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        if (ui::IsCtrlOrSuperDown())
        {
            if (e.keysym.mod & KMOD_SHIFT)
            {
                switch (e.keysym.sym) {
                case SDLK_z:  // Ctrl+Shift+Z : undo focused model
                    ActionRedoCurrentlyEditedModel(*m_Model);
                    return true;
                }
                return false;
            }

            switch (e.keysym.sym) {
            case SDLK_z:  // Ctrl+Z: undo focused model
                ActionUndoCurrentlyEditedModel(*m_Model);
                return true;
            case SDLK_r:
            {
                // Ctrl+R: start a new simulation from focused model
                return ActionStartSimulatingModel(m_Parent, *m_Model);
            }
            case SDLK_a:  // Ctrl+A: clear selection
                ActionClearSelectionFromEditedModel(*m_Model);
                return true;
            }

            return false;
        }

        switch (e.keysym.sym) {
        case SDLK_BACKSPACE:
        case SDLK_DELETE:  // BACKSPACE/DELETE: delete selection
            ActionTryDeleteSelectionFromEditedModel(*m_Model);
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

    void implPushPopup(std::unique_ptr<IPopup> popup) final
    {
        OSC_ASSERT(popup != nullptr);
        popup->open();
        m_PopupManager.push_back(std::move(popup));
    }

    void implAddMusclePlot(OpenSim::Coordinate const& coord, OpenSim::Muscle const& muscle) final
    {
        std::string const name = m_PanelManager->suggested_dynamic_panel_name("muscleplot");
        m_PanelManager->push_dynamic_panel(
            "muscleplot",
            std::make_shared<ModelMusclePlotPanel>(this, m_Model, name, GetAbsolutePath(coord), GetAbsolutePath(muscle))
        );
    }

    std::shared_ptr<PanelManager> implGetPanelManager() final
    {
        return m_PanelManager;
    }

    // tab top-level data
    UID m_TabID;
    ParentPtr<IMainUIStateAPI> m_Parent;
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
    PopupManager m_PopupManager;

    // flag that's set+reset each frame to prevent continual throwing
    bool m_ExceptionThrownLastFrame = false;
};


// public API (PIMPL)

osc::ModelEditorTab::ModelEditorTab(
    ParentPtr<IMainUIStateAPI> const& parent_,
    std::unique_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(parent_, std::move(model_))}
{
}

osc::ModelEditorTab::ModelEditorTab(ModelEditorTab&&) noexcept = default;
osc::ModelEditorTab& osc::ModelEditorTab::operator=(ModelEditorTab&&) noexcept = default;
osc::ModelEditorTab::~ModelEditorTab() noexcept = default;

UID osc::ModelEditorTab::impl_get_id() const
{
    return m_Impl->getID();
}

CStringView osc::ModelEditorTab::impl_get_name() const
{
    return m_Impl->getName();
}

bool osc::ModelEditorTab::impl_is_unsaved() const
{
    return m_Impl->isUnsaved();
}

bool osc::ModelEditorTab::impl_try_save()
{
    return m_Impl->trySave();
}

void osc::ModelEditorTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::ModelEditorTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::ModelEditorTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ModelEditorTab::impl_on_tick()
{
    m_Impl->on_tick();
}

void osc::ModelEditorTab::impl_on_draw_main_menu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ModelEditorTab::impl_on_draw()
{
    m_Impl->onDraw();
}

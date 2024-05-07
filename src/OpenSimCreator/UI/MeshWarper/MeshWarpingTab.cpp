#include "MeshWarpingTab.h"

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabInputMeshPanel.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabMainMenu.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabNavigatorPanel.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabResultMeshPanel.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabStatusBar.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabToolbar.h>

#include <SDL_events.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Panels/ToggleablePanelFlags.h>
#include <oscar/UI/Panels/UndoRedoPanel.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/UID.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::MeshWarpingTab::Impl final {
public:

    explicit Impl(ParentPtr<ITabHost> const& parent_) : m_Parent{parent_}
    {
        m_PanelManager->register_toggleable_panel(
            "Source Mesh",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabInputMeshPanel>(panelName, state, TPSDocumentInputIdentifier::Source);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "Destination Mesh",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabInputMeshPanel>(panelName, state, TPSDocumentInputIdentifier::Destination);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "Result",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabResultMeshPanel>(panelName, state);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "History",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<UndoRedoPanel>(panelName, state->editedDocument);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->register_toggleable_panel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->register_toggleable_panel(
            "Landmark Navigator",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabNavigatorPanel>(panelName, state);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->register_toggleable_panel(
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_BEZIER_CURVE " Mesh Warping";
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        m_PanelManager->on_mount();
        m_Shared->popupManager.on_mount();
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
        else
        {
            return false;
        }
    }

    void on_tick()
    {
        // re-perform hover test each frame
        m_Shared->currentHover.reset();

        // garbage collect panel data
        m_PanelManager->on_tick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ui::enable_dockspace_over_viewport(ui::get_main_viewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        m_TopToolbar.onDraw();
        m_PanelManager->on_draw();
        m_StatusBar.onDraw();

        // draw active popups over the UI
        m_Shared->popupManager.on_draw();
    }

private:
    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        bool const ctrlOrSuperDown = ui::is_ctrl_or_super_down();

        if (ctrlOrSuperDown && e.keysym.mod & KMOD_SHIFT && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Shift+Z: redo
            m_Shared->redo();
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Z: undo
            m_Shared->undo();
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_n)
        {
            // Ctrl+N: redo
            ActionCreateNewDocument(m_Shared->updUndoable());
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_w)
        {
            // Ctrl+W: new
            m_Shared->closeTab();
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_q)
        {
            // Ctrl+Q: quit
            App::upd().request_quit();
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_a)
        {
            // Ctrl+A: select all
            m_Shared->selectAll();
            return true;
        }
        else if (e.keysym.sym == SDLK_ESCAPE)
        {
            // ESCAPE: clear selection
            m_Shared->clearSelection();
            return true;
        }
        else
        {
            return false;
        }
    }

    UID m_TabID;
    ParentPtr<ITabHost> m_Parent;

    // top-level state that all panels can potentially access
    std::shared_ptr<MeshWarpingTabSharedState> m_Shared = std::make_shared<MeshWarpingTabSharedState>(m_TabID, m_Parent);

    // available/active panels that the user can toggle via the `window` menu
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();

    // not-user-toggleable widgets
    MeshWarpingTabMainMenu m_MainMenu{m_Shared, m_PanelManager};
    MeshWarpingTabToolbar m_TopToolbar{"##MeshWarpingTabToolbar", m_Shared};
    MeshWarpingTabStatusBar m_StatusBar{"##MeshWarpingTabStatusBar", m_Shared};
};


// public API (PIMPL)

CStringView osc::MeshWarpingTab::id()
{
    return "OpenSim/Warping";
}

osc::MeshWarpingTab::MeshWarpingTab(ParentPtr<ITabHost> const& parent_) :
    m_Impl{std::make_unique<Impl>(parent_)}
{
}

osc::MeshWarpingTab::MeshWarpingTab(MeshWarpingTab&&) noexcept = default;
osc::MeshWarpingTab& osc::MeshWarpingTab::operator=(MeshWarpingTab&&) noexcept = default;
osc::MeshWarpingTab::~MeshWarpingTab() noexcept = default;

UID osc::MeshWarpingTab::impl_get_id() const
{
    return m_Impl->getID();
}

CStringView osc::MeshWarpingTab::impl_get_name() const
{
    return m_Impl->getName();
}

void osc::MeshWarpingTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::MeshWarpingTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

bool osc::MeshWarpingTab::impl_on_event(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::MeshWarpingTab::impl_on_tick()
{
    m_Impl->on_tick();
}

void osc::MeshWarpingTab::impl_on_draw_main_menu()
{
    m_Impl->onDrawMainMenu();
}

void osc::MeshWarpingTab::impl_on_draw()
{
    m_Impl->onDraw();
}

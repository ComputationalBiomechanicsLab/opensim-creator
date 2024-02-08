#include "MeshWarpingTab.hpp"

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabInputMeshPanel.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabMainMenu.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabNavigatorPanel.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabResultMeshPanel.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabStatusBar.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabToolbar.hpp>

#include <SDL_events.h>
#include <oscar/UI/Panels/LogViewerPanel.hpp>
#include <oscar/UI/Panels/PanelManager.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Panels/ToggleablePanelFlags.hpp>
#include <oscar/UI/Panels/UndoRedoPanel.hpp>
#include <oscar/UI/Tabs/ITabHost.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>
#include <string_view>

using namespace osc;

class osc::MeshWarpingTab::Impl final {
public:

    explicit Impl(ParentPtr<ITabHost> const& parent_) : m_Parent{parent_}
    {
        m_PanelManager->registerToggleablePanel(
            "Source Mesh",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabInputMeshPanel>(panelName, state, TPSDocumentInputIdentifier::Source);
            }
        );

        m_PanelManager->registerToggleablePanel(
            "Destination Mesh",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabInputMeshPanel>(panelName, state, TPSDocumentInputIdentifier::Destination);
            }
        );

        m_PanelManager->registerToggleablePanel(
            "Result",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabResultMeshPanel>(panelName, state);
            }
        );

        m_PanelManager->registerToggleablePanel(
            "History",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<UndoRedoPanel>(panelName, state->editedDocument);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->registerToggleablePanel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->registerToggleablePanel(
            "Landmark Navigator",
            [state = m_Shared](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabNavigatorPanel>(panelName, state);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->registerToggleablePanel(
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

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_PanelManager->onMount();
        m_Shared->popupManager.onMount();
    }

    void onUnmount()
    {
        m_PanelManager->onUnmount();
        App::upd().makeMainEventLoopPolling();
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

    void onTick()
    {
        // re-perform hover test each frame
        m_Shared->currentHover.reset();

        // garbage collect panel data
        m_PanelManager->onTick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        m_TopToolbar.onDraw();
        m_PanelManager->onDraw();
        m_StatusBar.onDraw();

        // draw active popups over the UI
        m_Shared->popupManager.onDraw();
    }

private:
    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        bool const ctrlOrSuperDown = IsCtrlOrSuperDown();

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
            App::upd().requestQuit();
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

UID osc::MeshWarpingTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::MeshWarpingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::MeshWarpingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::MeshWarpingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::MeshWarpingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::MeshWarpingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::MeshWarpingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::MeshWarpingTab::implOnDraw()
{
    m_Impl->onDraw();
}

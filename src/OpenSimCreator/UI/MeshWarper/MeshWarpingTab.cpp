#include "MeshWarpingTab.hpp"

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabInputMeshPanel.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabMainMenu.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabNavigatorPanel.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabResultMeshPanel.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabStatusBar.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabToolbar.hpp>

#include <oscar/UI/Panels/PanelManager.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Panels/ToggleablePanelFlags.hpp>
#include <oscar/UI/Panels/LogViewerPanel.hpp>
#include <oscar/UI/Panels/UndoRedoPanel.hpp>
#include <oscar/UI/Tabs/TabHost.hpp>
#include <oscar/Utils/UID.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <SDL_events.h>

#include <memory>
#include <string_view>

class osc::MeshWarpingTab::Impl final {
public:

    explicit Impl(ParentPtr<TabHost> const& parent_) : m_Parent{parent_}
    {
        m_PanelManager->registerToggleablePanel(
            "Source Mesh",
            [state = m_SharedState](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabInputMeshPanel>(panelName, state, TPSDocumentInputIdentifier::Source);
            }
        );

        m_PanelManager->registerToggleablePanel(
            "Destination Mesh",
            [state = m_SharedState](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabInputMeshPanel>(panelName, state, TPSDocumentInputIdentifier::Destination);
            }
        );

        m_PanelManager->registerToggleablePanel(
            "Result",
            [state = m_SharedState](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabResultMeshPanel>(panelName, state);
            }
        );

        m_PanelManager->registerToggleablePanel(
            "History",
            [state = m_SharedState](std::string_view panelName)
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
            "Performance",
            [](std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(panelName);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->registerToggleablePanel(
            "Navigator",
            [state = m_SharedState](std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabNavigatorPanel>(panelName, state);
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
        m_SharedState->popupManager.onMount();
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
        m_SharedState->currentHover.reset();

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
        m_SharedState->popupManager.onDraw();
    }

private:
    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        bool const ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

        if (ctrlOrSuperDown && e.keysym.mod & KMOD_SHIFT && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Shift+Z: redo
            ActionRedo(*m_SharedState->editedDocument);
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Z: undo
            ActionUndo(*m_SharedState->editedDocument);
            return true;
        }
        else
        {
            return false;
        }
    }

    UID m_TabID;
    ParentPtr<TabHost> m_Parent;

    // top-level state that all panels can potentially access
    std::shared_ptr<MeshWarpingTabSharedState> m_SharedState = std::make_shared<MeshWarpingTabSharedState>(m_TabID, m_Parent);

    // available/active panels that the user can toggle via the `window` menu
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();

    // not-user-toggleable widgets
    MeshWarpingTabMainMenu m_MainMenu{m_SharedState, m_PanelManager};
    MeshWarpingTabToolbar m_TopToolbar{"##MeshWarpingTabToolbar", m_SharedState};
    MeshWarpingTabStatusBar m_StatusBar{"##MeshWarpingTabStatusBar", m_SharedState};
};


// public API (PIMPL)

osc::CStringView osc::MeshWarpingTab::id()
{
    return "OpenSim/Warping";
}

osc::MeshWarpingTab::MeshWarpingTab(ParentPtr<TabHost> const& parent_) :
    m_Impl{std::make_unique<Impl>(parent_)}
{
}

osc::MeshWarpingTab::MeshWarpingTab(MeshWarpingTab&&) noexcept = default;
osc::MeshWarpingTab& osc::MeshWarpingTab::operator=(MeshWarpingTab&&) noexcept = default;
osc::MeshWarpingTab::~MeshWarpingTab() noexcept = default;

osc::UID osc::MeshWarpingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::MeshWarpingTab::implGetName() const
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

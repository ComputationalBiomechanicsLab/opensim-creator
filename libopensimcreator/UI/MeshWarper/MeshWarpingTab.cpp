#include "MeshWarpingTab.h"

#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabInputMeshPanel.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabMainMenu.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabNavigatorPanel.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabResultMeshPanel.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabStatusBar.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabToolbar.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/panels/panel_manager.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/panels/toggleable_panel_flags.h>
#include <liboscar/ui/panels/undo_redo_panel.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utils/UID.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::MeshWarpingTab::Impl final : public TabPrivate {
public:

    explicit Impl(MeshWarpingTab& owner, Widget* parent_) :
        TabPrivate{owner, parent_, MSMICONS_BEZIER_CURVE " Mesh Warping"},
        m_Shared{std::make_shared<MeshWarpingTabSharedState>(id(), &owner, App::singleton<SceneCache>(App::resource_loader()))}
    {
        m_PanelManager->register_toggleable_panel(
            "Source Mesh",
            [state = m_Shared](Widget* parent, std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabInputMeshPanel>(parent, panelName, state, TPSDocumentInputIdentifier::Source);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "Destination Mesh",
            [state = m_Shared](Widget* parent, std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabInputMeshPanel>(parent, panelName, state, TPSDocumentInputIdentifier::Destination);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "Result",
            [state = m_Shared](Widget* parent, std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabResultMeshPanel>(parent, panelName, state);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "History",
            [state = m_Shared](Widget* parent, std::string_view panelName)
            {
                return std::make_shared<UndoRedoPanel>(parent, panelName, state->getUndoableSharedPtr());
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->register_toggleable_panel(
            "Log",
            [](Widget* parent, std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(parent, panelName);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->register_toggleable_panel(
            "Landmark Navigator",
            [state = m_Shared](Widget* parent, std::string_view panelName)
            {
                return std::make_shared<MeshWarpingTabNavigatorPanel>(parent, panelName, state);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );

        m_PanelManager->register_toggleable_panel(
            "Performance",
            [](Widget* parent, std::string_view panelName)
            {
                return std::make_shared<PerfPanel>(parent, panelName);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault
        );
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        m_PanelManager->on_mount();
        m_Shared->on_mount();
    }

    void on_unmount()
    {
        m_Shared->on_unmount();
        m_PanelManager->on_unmount();
        App::upd().make_main_loop_polling();
    }

    bool onEvent(Event& e)
    {
        if (e.type() == EventType::KeyDown) {
            return onKeydownEvent(dynamic_cast<const KeyEvent&>(e));
        }
        else {
            return false;
        }
    }

    void on_tick()
    {
        // re-perform hover test each frame
        m_Shared->setHover(std::nullopt);

        // garbage collect panel data
        m_PanelManager->on_tick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.onDraw();
    }

    void onDraw()
    {
        ui::enable_dockspace_over_main_window();

        m_TopToolbar.on_draw();
        m_PanelManager->on_draw();
        m_StatusBar.onDraw();
        m_Shared->on_draw();
    }

private:
    bool onKeydownEvent(const KeyEvent& e)
    {
        if (e.combination() == (KeyModifier::Ctrl | KeyModifier::Shift | Key::Z)) {
            m_Shared->redo();
            return true;
        }
        else if (e.combination() == (KeyModifier::Ctrl | Key::Z)) {
            m_Shared->undo();
            return true;
        }
        else if (e.combination() == (KeyModifier::Ctrl | Key::N)) {
            ActionCreateNewDocument(m_Shared->updUndoable());
            return true;
        }
        else if (e.combination() == (KeyModifier::Ctrl | Key::Q)) {
            App::upd().request_quit();
            return true;
        }
        else if (e.combination() == (KeyModifier::Ctrl | Key::A)) {
            m_Shared->selectAll();
            return true;
        }
        else if (e.combination() == Key::Escape) {
            m_Shared->clearSelection();
            return true;
        }
        else {
            return false;
        }
    }

    // top-level state that all panels can potentially access
    std::shared_ptr<MeshWarpingTabSharedState> m_Shared;

    // available/active panels that the user can toggle via the `window` menu
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>(&owner());

    // not-user-toggleable widgets
    MeshWarpingTabMainMenu m_MainMenu{&owner(), m_Shared, m_PanelManager};
    MeshWarpingTabToolbar m_TopToolbar{&owner(), "##MeshWarpingTabToolbar", m_Shared};
    MeshWarpingTabStatusBar m_StatusBar{"##MeshWarpingTabStatusBar", m_Shared};
};


CStringView osc::MeshWarpingTab::id() { return "OpenSim/Warping"; }

osc::MeshWarpingTab::MeshWarpingTab(Widget* parent_) :
    Tab{std::make_unique<Impl>(*this, parent_)}
{}
void osc::MeshWarpingTab::impl_on_mount() { private_data().on_mount(); }
void osc::MeshWarpingTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::MeshWarpingTab::impl_on_event(Event& e) { return private_data().onEvent(e); }
void osc::MeshWarpingTab::impl_on_tick() { private_data().on_tick(); }
void osc::MeshWarpingTab::impl_on_draw_main_menu() { private_data().onDrawMainMenu(); }
void osc::MeshWarpingTab::impl_on_draw() { private_data().onDraw(); }

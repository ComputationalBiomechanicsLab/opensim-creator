#include "mw_ui_tab.h"

#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_source_mesh_panel.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_main_menu.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_landmark_navigator_panel.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_result_panel.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_status_bar.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_toolbar.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/panels/panel_manager.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/panels/toggleable_panel_flags.h>
#include <liboscar/ui/panels/undo_redo_panel.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utilities/uid.h>

#include <memory>
#include <string_view>

using namespace osc;

class osc::MwUiTab::Impl final : public TabPrivate {
public:
    explicit Impl(MwUiTab& owner, Widget* parent) :
        TabPrivate{owner, parent, MSMICONS_BEZIER_CURVE " Mesh Warping"},
        shared_{std::make_shared<MwUiSharedState>(
            id(), &owner, App::singleton<SceneCache>(App::resource_loader()))}
    {
        panel_manager_->register_toggleable_panel(
            "Source Mesh", [state = shared_](Widget* parent, std::string_view panel_name) {
                return std::make_shared<MwUiSourceMeshPanel>(
                    parent, panel_name, state, MiDocumentInputIdentifier::Source);
            });

        panel_manager_->register_toggleable_panel(
            "Destination Mesh", [state = shared_](Widget* parent, std::string_view panel_name) {
                return std::make_shared<MwUiSourceMeshPanel>(
                    parent, panel_name, state, MiDocumentInputIdentifier::Destination);
            });

        panel_manager_->register_toggleable_panel(
            "Result", [state = shared_](Widget* parent, std::string_view panel_name) {
                return std::make_shared<MwUiResultPanel>(parent, panel_name, state);
            });

        panel_manager_->register_toggleable_panel(
            "History",
            [state = shared_](Widget* parent, std::string_view panel_name) {
                return std::make_shared<UndoRedoPanel>(
                    parent, panel_name, state->get_undoable_shared_ptr());
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault);

        panel_manager_->register_toggleable_panel(
            "Log",
            [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<LogViewerPanel>(parent, panel_name);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault);

        panel_manager_->register_toggleable_panel(
            "Landmark Navigator",
            [state = shared_](Widget* parent, std::string_view panel_name) {
                return std::make_shared<MwUiLandmarkNavigatorPanel>(parent, panel_name, state);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault);

        panel_manager_->register_toggleable_panel(
            "Performance",
            [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<PerfPanel>(parent, panel_name);
            },
            ToggleablePanelFlags::Default - ToggleablePanelFlags::IsEnabledByDefault);
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        panel_manager_->on_mount();
        shared_->on_mount();
    }

    void on_unmount()
    {
        shared_->on_unmount();
        panel_manager_->on_unmount();
        App::upd().make_main_loop_polling();
    }

    bool on_event(Event& e)
    {
        if (e.type() == EventType::KeyDown) {
            return on_keydown_event(dynamic_cast<const KeyEvent&>(e));
        } else {
            return false;
        }
    }

    void on_tick()
    {
        // re-perform hover test each frame
        shared_->set_hover(std::nullopt);

        // garbage collect panel data
        panel_manager_->on_tick();
    }

    void on_draw_main_menu() { main_menu_.on_draw(); }

    void on_draw()
    {
        ui::enable_dockspace_over_main_window();

        top_toolbar_.on_draw();
        panel_manager_->on_draw();
        status_bar_.on_draw();
        shared_->on_draw();
    }

private:
    bool on_keydown_event(const KeyEvent& e)
    {
        if (e.combination() == (ModifierKey::Ctrl | ModifierKey::Shift | Key::Z)) {
            shared_->redo();
            return true;
        } else if (e.combination() == (ModifierKey::Ctrl | Key::Z)) {
            shared_->undo();
            return true;
        } else if (e.combination() == (ModifierKey::Ctrl | Key::N)) {
            action_create_new_document(shared_->upd_undoable());
            return true;
        } else if (e.combination() == (ModifierKey::Ctrl | Key::Q)) {
            App::upd().request_quit();
            return true;
        } else if (e.combination() == (ModifierKey::Ctrl | Key::A)) {
            shared_->select_all();
            return true;
        } else if (e.combination() == Key::Escape) {
            shared_->clear_selection();
            return true;
        } else {
            return false;
        }
    }

    // top-level state that all panels can potentially access
    std::shared_ptr<MwUiSharedState> shared_;

    // available/active panels that the user can toggle via the `window` menu
    std::shared_ptr<PanelManager> panel_manager_ = std::make_shared<PanelManager>(&owner());

    // not-user-toggleable widgets
    MwUiMainMenu main_menu_{&owner(), shared_, panel_manager_};
    MwUiToolbar top_toolbar_{&owner(), "##MeshWarpingTabToolbar", shared_};
    MwUiStatusBar status_bar_{"##MeshWarpingTabStatusBar", shared_};
};

CStringView osc::MwUiTab::id()
{
    return "OpenSim/Warping";
}

osc::MwUiTab::MwUiTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::MwUiTab::impl_on_mount()
{
    private_data().on_mount();
}
void osc::MwUiTab::impl_on_unmount()
{
    private_data().on_unmount();
}
bool osc::MwUiTab::impl_on_event(Event& e)
{
    return private_data().on_event(e);
}
void osc::MwUiTab::impl_on_tick()
{
    private_data().on_tick();
}
void osc::MwUiTab::impl_on_draw_main_menu()
{
    private_data().on_draw_main_menu();
}
void osc::MwUiTab::impl_on_draw()
{
    private_data().on_draw();
}

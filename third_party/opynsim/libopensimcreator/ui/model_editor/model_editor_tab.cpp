#include "model_editor_tab.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/loading_tab.h>
#include <libopensimcreator/ui/model_editor/model_editor_main_menu.h>
#include <libopensimcreator/ui/model_editor/model_editor_toolbar.h>
#include <libopensimcreator/ui/model_editor/model_muscle_plot_panel.h>
#include <libopensimcreator/ui/shared/add_muscle_plot_event.h>
#include <libopensimcreator/ui/shared/component_context_menu.h>
#include <libopensimcreator/ui/shared/coordinate_editor_panel.h>
#include <libopensimcreator/ui/shared/model_status_bar.h>
#include <libopensimcreator/ui/shared/model_viewer_panel.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_parameters.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_right_click_event.h>
#include <libopensimcreator/ui/shared/navigator_panel.h>
#include <libopensimcreator/ui/shared/open_component_context_menu_event.h>
#include <libopensimcreator/ui/shared/output_watches_panel.h>
#include <libopensimcreator/ui/shared/properties_panel.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/events/drop_file_event.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/events/close_tab_event.h>
#include <liboscar/ui/events/open_named_panel_event.h>
#include <liboscar/ui/events/open_panel_event.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/events/reset_ui_context_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/log_viewer_panel.h>
#include <liboscar/ui/panels/panel_manager.h>
#include <liboscar/ui/panels/perf_panel.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_manager.h>
#include <liboscar/ui/tabs/error_tab.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/exception_helpers.h>
#include <liboscar/utilities/file_change_poller.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

class osc::ModelEditorTab::Impl final : public TabPrivate {
public:
    explicit Impl(ModelEditorTab& owner, Widget* parent) :
        Impl{owner, parent, std::make_unique<UndoableModelStatePair>()}
    {}

    explicit Impl(ModelEditorTab& owner, Widget* parent, const OpenSim::Model& model) :
        Impl{owner, parent, std::make_unique<UndoableModelStatePair>(model)}
    {}

    explicit Impl(
        ModelEditorTab& owner,
        Widget* parent,
        std::unique_ptr<OpenSim::Model> model,
        float fixup_scale_factor) :
        Impl{owner, parent, std::make_unique<UndoableModelStatePair>(std::move(model))}
    {
        model_->set_fixup_scale_factor(fixup_scale_factor);
    }

    explicit Impl(
        ModelEditorTab& owner, Widget* parent, std::unique_ptr<UndoableModelStatePair> model) :

        TabPrivate{owner, parent, "ModelEditorTab"},
        model_{std::move(model)}
    {
        // register all panels that the editor tab supports

        panel_manager_->register_toggleable_panel(
            "Navigator", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<NavigatorPanel>(
                    parent, panel_name, model_, [this](const OpenSim::ComponentPath& p) {
                        auto popup = std::make_unique<ComponentContextMenu>(
                            &this->owner(), "##componentcontextmenu", model_, p);
                        App::post_event<OpenPopupEvent>(this->owner(), std::move(popup));
                    });
            });
        panel_manager_->register_toggleable_panel(
            "Properties", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<PropertiesPanel>(parent, panel_name, model_);
            });
        panel_manager_->register_toggleable_panel(
            "Log", [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<LogViewerPanel>(parent, panel_name);
            });
        panel_manager_->register_toggleable_panel(
            "Coordinates", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<CoordinateEditorPanel>(parent, panel_name, model_);
            });
        panel_manager_->register_toggleable_panel(
            "Performance", [](Widget* parent, std::string_view panel_name) {
                return std::make_shared<PerfPanel>(parent, panel_name);
            });
        panel_manager_->register_toggleable_panel(
            "Output Watches", [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<OutputWatchesPanel>(parent, panel_name, model_);
            });
        panel_manager_->register_spawnable_panel(
            "viewer",
            [this](Widget*, std::string_view panel_name) {
                auto on_right_click =
                    [this, model = model_, menu_name = std::string{panel_name} + "_contextmenu"](
                        const ModelViewerPanelRightClickEvent& e) {
                        auto popup = std::make_unique<ComponentContextMenu>(
                            &this->owner(), menu_name, model, e.component_abs_path_or_empty);

                        App::post_event<OpenPopupEvent>(this->owner(), std::move(popup));
                    };
                const ModelViewerPanelParameters panel_params{model_, on_right_click};

                return std::make_shared<ModelViewerPanel>(&this->owner(), panel_name, panel_params);
            },
            1  // have one viewer open at the start
        );
        panel_manager_->register_spawnable_panel(
            "muscleplot",
            [this](Widget* parent, std::string_view panel_name) {
                return std::make_shared<ModelMusclePlotPanel>(parent, model_, panel_name);
            },
            0  // no muscle plots open at the start
        );
    }

    bool is_unsaved() const { return !model_->is_up_to_date_with_filesystem(); }

    std::future<TabSaveResult> try_save()
    {
        auto promise = std::make_shared<std::promise<TabSaveResult>>();
        std::future<TabSaveResult> future = promise->get_future();
        action_save_model_async(model_, [promise](bool ok) mutable {
            promise->set_value(ok ? TabSaveResult::Done : TabSaveResult::Cancelled);
        });
        return future;
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        App::upd().set_main_window_subtitle(opyn::recommended_document_name(model_->get_model()));
        set_name(compute_tab_name());
        popup_manager_.on_mount();
        panel_manager_->on_mount();
    }

    void on_unmount()
    {
        panel_manager_->on_unmount();
        App::upd().set_main_window_subtitle(std::nullopt);
        App::upd().make_main_loop_polling();
    }

    bool on_event(Event& e)
    {
        try {
            return on_event_unguarded(e);
        } catch (const std::exception& ex) {
            try_recovering_from_exception(ex);
            return false;
        }
    }

    bool on_event_unguarded(Event& e)
    {
        if (auto* open_popup_event = dynamic_cast<OpenPopupEvent*>(&e)) {
            if (open_popup_event->has_popup()) {
                auto popup = open_popup_event->take_popup();
                popup->set_parent(&owner());
                popup->open();
                popup_manager_.push_back(std::move(popup));
                return true;
            }
        } else if (const auto* named_panel = dynamic_cast<OpenNamedPanelEvent*>(&e)) {
            panel_manager_->set_toggleable_panel_activated(named_panel->panel_name(), true);
            return true;
        } else if (auto* panel = dynamic_cast<OpenPanelEvent*>(&e)) {
            if (panel->has_panel()) {
                auto panel_ptr = panel->take_panel();
                panel_ptr->set_parent(&owner());
                const std::string name{panel_ptr->name()};
                panel_manager_->push_dynamic_panel(name, std::move(panel_ptr));
                return true;
            }
        } else if (
            const auto* context_menu_event = dynamic_cast<OpenComponentContextMenuEvent*>(&e)) {

            auto popup = std::make_unique<ComponentContextMenu>(
                &this->owner(), "##componentcontextmenu", model_, context_menu_event->path());
            App::post_event<OpenPopupEvent>(owner(), std::move(popup));
            return true;
        } else if (const auto* add_muscle_plot_event = dynamic_cast<AddMusclePlotEvent*>(&e)) {
            const std::string name = panel_manager_->suggested_dynamic_panel_name("muscleplot");

            panel_manager_->push_dynamic_panel(
                "muscleplot",
                std::make_shared<ModelMusclePlotPanel>(
                    &owner(),
                    model_,
                    name,
                    add_muscle_plot_event->get_coordinate_abs_path(),
                    add_muscle_plot_event->get_muscle_abs_path()));
            return true;
        }

        switch (e.type()) {
        case EventType::KeyDown: return on_keydown_event(dynamic_cast<const KeyEvent&>(e));
        case EventType::DropFile: return on_drop_event(dynamic_cast<const DropFileEvent&>(e));
        default: return false;
        }
    }

    void on_tick()
    {
        // If the user has defined auto-reload behavior, obey it. Otherwise, default-enable
        // auto-reloading (#1000)
        if (App::settings().find_value<bool>("model_editor/monitor_osim_changes").value_or(true)) {
            if (file_change_poller_.change_detected(model_->get_model().getInputFileName())) {
                action_update_model_from_backing_file(*model_);
            }
        }

        set_name(compute_tab_name());
        panel_manager_->on_tick();
    }

    void on_draw_main_menu() { main_menu_.on_draw(); }

    void on_draw()
    {
        ui::enable_dockspace_over_main_window();

        try {
            toolbar_.on_draw();
            panel_manager_->on_draw();
            status_bar_.on_draw();
            popup_manager_.on_draw();

            exception_thrown_last_frame_ = false;
        } catch (const std::exception& ex) {
            try_recovering_from_exception(ex);

            // Request to reset the 2D UI context, because the exception
            // unroll may have left it in an indeterminate state.
            if (parent()) {
                App::notify<ResetUIContextEvent>(*parent());
            }
        }

        // always re-update this, in case the model's document name changed
        App::upd().set_main_window_subtitle(opyn::recommended_document_name(model_->get_model()));
    }

    void try_recovering_from_exception(const std::exception& ex)
    {
        log_error(
            "exception: thrown while drawing the model editor\n{}",
            potentially_nested_exception_to_string(ex, 1));
        log_error("Exceptions typically happen when an invalid edit is made to the model");

        if (exception_thrown_last_frame_) {
            if (model_->can_undo()) {
                // exception was thrown last frame, indicating the model in the undo/redo buffer is
                // also damaged, so try undoing

                log_error(
                    "an exception was also thrown last frame, indicating model damage: attempting to undo to an earlier version of the model to try and fix the model");

                try {
                    // TODO: add `doUndoWithNoRedoStorage` so that the user's redo buffer isn't
                    // tainted
                    model_->do_undo();
                } catch (const std::exception& ex2) {
                    log_error("undoing the model also failed with an error");
                    log_error(potentially_nested_exception_to_string(ex2, 1));
                    log_error("because the model isn't recoverable, closing the editor tab");
                    if (parent()) {
                        App::post_event<OpenTabEvent>(
                            *parent(), std::make_unique<ErrorTab>(owner(), ex));
                        App::post_event<CloseTabEvent>(*parent(), id());
                    }
                }

                log_error("successfully undone model");
                exception_thrown_last_frame_ = false;  // reset flag
            } else if (not popup_manager_.empty()) {
                // exception was thrown last frame, but we can't undo the model, so try to assume
                // that a popup was causing the problem last frame and clear all popups instead of
                // fully exploding the whole tab
                log_error(
                    "trying to close all currently-open popups, in case that prevents crashes");
                popup_manager_.clear();
            } else {
                // exception thrown last frame, indicating the model in the undo/redo buffer is also
                // damaged, but cannot undo, so quit

                log_error("because the model isn't recoverable, closing the editor tab");
                if (parent()) {
                    App::post_event<OpenTabEvent>(
                        *parent(), std::make_unique<ErrorTab>(owner(), ex));
                    App::post_event<CloseTabEvent>(*parent(), id());
                }
            }
        } else {
            // no exception last frame, indicating the _scratch space_ may be damaged, so try to
            // rollback to a version in the undo/redo buffer

            try {
                log_error("attempting to rollback the model edit to a clean state");
                model_->rollback();
                log_error("model rollback succeeded");
                exception_thrown_last_frame_ = true;
            } catch (const std::exception& ex2) {
                log_error("model rollback thrown an exception: {}", ex2.what());
                log_error("because the model cannot be rolled back, closing the editor tab");
                App::post_event<OpenTabEvent>(*parent(), std::make_unique<ErrorTab>(owner(), ex2));
                App::post_event<CloseTabEvent>(*parent(), id());
            }
        }
    }

private:
    std::string compute_tab_name()
    {
        std::stringstream ss;
        ss << MSMICONS_EDIT << " ";
        ss << opyn::recommended_document_name(model_->get_model());
        return std::move(ss).str();
    }

    bool on_drop_event(const DropFileEvent& e)
    {
        if (e.path().extension() == ".sto") {
            return action_load_sto_file_against_model(owner(), *model_, e.path());
        } else if (opyn::has_model_file_extension(e.path())) {
            // if the user drops an osim file on this tab then it should be loaded
            auto tab = std::make_unique<LoadingTab>(&owner(), e.path());
            App::post_event<OpenTabEvent>(owner(), std::move(tab));
            return true;
        }

        return false;
    }

    bool on_keydown_event(const KeyEvent& e)
    {
        if (e.combination() == (ModifierKey::Ctrl | ModifierKey::Shift | Key::Z)) {
            model_->do_redo();
            return true;
        } else if (e.combination() == (ModifierKey::Ctrl | Key::Z)) {
            model_->do_undo();
            return true;
        } else if (e.combination() == (ModifierKey::Ctrl | Key::R)) {
            return action_start_simulating_model(owner(), *model_);
        } else if (e.combination() == Key::Backspace or e.combination() == Key::Delete) {
            action_try_delete_selection_from_edited_model(*model_);
            return true;
        } else if (e.combination() == Key::Escape) {
            model_->clear_selected();
            return true;
        } else {
            return false;
        }
    }

    // the model being edited
    std::shared_ptr<UndoableModelStatePair> model_;

    // polls changes to a file
    FileChangePoller file_change_poller_{
        std::chrono::milliseconds{1000},  // polling rate
        model_->get_model().getInputFileName(),
    };

    // manager for toggleable and spawnable UI panels
    std::shared_ptr<PanelManager> panel_manager_ = std::make_shared<PanelManager>(&owner());

    // non-toggleable UI panels/menus/toolbars
    ModelEditorMainMenu main_menu_{&owner(), panel_manager_, model_};
    ModelEditorToolbar toolbar_{&owner(), "##ModelEditorToolbar", model_};
    ModelStatusBar status_bar_{&owner(), model_};

    // manager for popups that are open in this tab
    PopupManager popup_manager_;

    // flag that's set+reset each frame to prevent continual throwing
    bool exception_thrown_last_frame_ = false;
};

osc::ModelEditorTab::ModelEditorTab(Widget* parent) :

    Tab{std::make_unique<Impl>(*this, parent)}
{}
osc::ModelEditorTab::ModelEditorTab(Widget* parent, const OpenSim::Model& model) :

    Tab{std::make_unique<Impl>(*this, parent, model)}
{}
osc::ModelEditorTab::ModelEditorTab(
    Widget* parent, std::unique_ptr<OpenSim::Model> model, float fixup_scale_factor) :

    Tab{std::make_unique<Impl>(*this, parent, std::move(model), fixup_scale_factor)}
{}
osc::ModelEditorTab::ModelEditorTab(Widget* parent, std::unique_ptr<UndoableModelStatePair> model) :

    Tab{std::make_unique<Impl>(*this, parent, std::move(model))}
{}
bool osc::ModelEditorTab::impl_is_unsaved() const
{
    return private_data().is_unsaved();
}
std::future<TabSaveResult> osc::ModelEditorTab::impl_try_save()
{
    return private_data().try_save();
}
void osc::ModelEditorTab::impl_on_mount()
{
    private_data().on_mount();
}
void osc::ModelEditorTab::impl_on_unmount()
{
    private_data().on_unmount();
}
bool osc::ModelEditorTab::impl_on_event(Event& e)
{
    return private_data().on_event(e);
}
void osc::ModelEditorTab::impl_on_tick()
{
    private_data().on_tick();
}
void osc::ModelEditorTab::impl_on_draw_main_menu()
{
    private_data().on_draw_main_menu();
}
void osc::ModelEditorTab::impl_on_draw()
{
    private_data().on_draw();
}

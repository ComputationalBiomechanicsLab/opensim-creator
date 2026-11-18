#include "model_editor_main_menu.h"

#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/model_editor/export_points_popup.h>
#include <libopensimcreator/ui/model_editor/model_muscle_plot_panel.h>
#include <libopensimcreator/ui/shared/import_stations_from_csv_popup.h>
#include <libopensimcreator/ui/shared/main_menu.h>
#include <libopensimcreator/ui/shared/model_add_menu_items.h>
#include <libopensimcreator/ui/shared/param_block_editor_popup.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/widgets/window_menu.h>

#include <memory>
#include <utility>

using namespace osc;

class osc::ModelEditorMainMenu::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner,
        Widget* parent,
        std::shared_ptr<PanelManager> panel_manager,
        std::shared_ptr<ModelStatePairWithSharedEnvironment> model) :

        WidgetPrivate{owner, parent},
        model_{std::move(model)},
        main_menu_file_tab_{&owner},
        window_menu_{&owner, std::move(panel_manager)}
    {}

    void on_draw()
    {
        main_menu_file_tab_.on_draw(model_);
        draw_main_menu_edit_tab();
        draw_main_menu_add_tab();
        draw_main_menu_tools_tab();
        draw_main_menu_actions_tab();
        window_menu_.on_draw();
        main_menu_about_tab_.on_draw();
    }

private:
    void draw_main_menu_edit_tab()
    {
        if (ui::begin_menu("Edit")) {
            auto* undoable = dynamic_cast<UndoableModelStatePair*>(model_.get());
            if (ui::draw_menu_item(
                    MSMICONS_UNDO " Undo",
                    ModifierKey::Ctrl | Key::Z,
                    false,
                    undoable != nullptr and undoable->can_undo())) {
                if (undoable) {
                    undoable->do_undo();
                }
            }

            if (ui::draw_menu_item(
                    MSMICONS_REDO " Redo",
                    ModifierKey::Ctrl | ModifierKey::Shift | Key::Z,
                    false,
                    undoable != nullptr and undoable->can_redo())) {
                if (undoable) {
                    undoable->do_redo();
                }
            }

            ui::draw_separator();

            if (ui::draw_menu_item(
                    "         Deselect", Key::Escape, false, model_->get_selected() != nullptr)) {
                model_->clear_selected();
            }

            ui::end_menu();
        }
    }

    void draw_main_menu_add_tab()
    {
        if (ui::begin_menu("Add")) {
            main_menu_model_add_menu_items_.on_draw();
            ui::end_menu();
        }
    }

    void draw_main_menu_tools_tab()
    {
        if (ui::begin_menu("Tools")) {
            if (ui::draw_menu_item(MSMICONS_PLAY " Simulate", ModifierKey::Ctrl | Key::R)) {
                action_start_simulating_model(owner(), *model_);
            }

            if (ui::draw_menu_item(MSMICONS_EDIT " Edit simulation settings")) {
                if (parent()) {
                    auto popup = std::make_unique<ParamBlockEditorPopup>(
                        &owner(),
                        "simulation parameters",
                        &model_->try_upd_environment()->upd_simulation_params());
                    App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
                }
            }

            if (ui::draw_menu_item(
                    "         Import Points", {}, nullptr, model_->can_upd_model())) {
                if (parent()) {
                    auto popup = std::make_unique<ImportStationsFromCSVPopup>(
                        &owner(),
                        "Import Points",
                        [model = model_](auto lms) {
                            action_import_landmarks(
                                *model,
                                lms.landmarks,
                                lms.maybe_label,
                                lms.maybe_target_component_abs_path);
                        },
                        model_);
                    App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
                }
            }

            if (ui::draw_menu_item("         Export Points")) {
                auto popup = std::make_unique<ExportPointsPopup>(&owner(), "Export Points", model_);
                if (parent()) {
                    App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
                }
            }

            if (ui::begin_menu("         Experimental Tools")) {
                if (ui::draw_menu_item("Export Model Graph as Dotviz")) {
                    action_export_model_graph_to_dotviz(model_);
                }
                ui::draw_tooltip_if_item_hovered(
                    "Writes the model's data topology graph in dotviz format, so that it can be visualized in external tooling such as Graphviz Online");

                if (ui::draw_menu_item("Export Model Graph as Dotviz (clipboard)")) {
                    action_export_model_graph_to_dotviz_clipboard(*model_);
                }

                if (ui::draw_menu_item("Export Model Multibody System as Dotviz (clipboard)")) {
                    action_export_model_multibody_system_as_dotviz(*model_);
                }
                ui::draw_tooltip_if_item_hovered(
                    "Writes the model's multibody system (kinematic chain) in dotviz format, so that it can be visualized in external tooling such as Graphviz Online");

                if (ui::draw_menu_item("WIP: Bake Station Defined Frames")) {
                    action_bake_station_defined_frames(*model_);
                }
                ui::draw_tooltip_if_item_hovered(
                    "WORK IN PROGRESS (WIP): Converts any `StationDefinedFrame`s in the model into `PhysicalOffsetFrame`s. Effectively, \"baking\" the current (station-defined) frame transform.\n\nThe main reason to do this is backwards compatibility, OpenSim <= v4.5 doesn't have native support for `StationDefinedFrame`s (later versions should: see opensim-core/#3694)");

                ui::end_menu();
            }

            ui::end_menu();
        }
    }

    void draw_main_menu_actions_tab()
    {
        if (ui::begin_menu("Actions")) {
            if (ui::draw_menu_item(
                    "Disable all wrapping surfaces", {}, nullptr, model_->can_upd_model())) {
                action_disable_all_wrapping_surfaces(*model_);
            }

            if (ui::draw_menu_item(
                    "Enable all wrapping surfaces", {}, nullptr, model_->can_upd_model())) {
                action_enable_all_wrapping_surfaces(*model_);
            }

            ui::end_menu();
        }
    }

    std::shared_ptr<ModelStatePairWithSharedEnvironment> model_;
    MainMenuFileTab main_menu_file_tab_;
    ModelAddMenuItems main_menu_model_add_menu_items_{&owner(), model_};
    WindowMenu window_menu_;
    MainMenuAboutTab main_menu_about_tab_;
};

osc::ModelEditorMainMenu::ModelEditorMainMenu(
    Widget* parent,
    std::shared_ptr<PanelManager> panel_manager,
    std::shared_ptr<ModelStatePairWithSharedEnvironment> model) :

    Widget{std::make_unique<Impl>(*this, parent, std::move(panel_manager), std::move(model))}
{}

void osc::ModelEditorMainMenu::impl_on_draw()
{
    private_data().on_draw();
}

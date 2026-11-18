#include "mw_ui_toolbar.h"

#include <libopensimcreator/documents/mesh_warper/mw_undoable_actions.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>

#include <libopynsim/documents/landmarks/named_landmark.h>
#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/widgets/redo_button.h>
#include <liboscar/ui/widgets/undo_button.h>

#include <memory>
#include <string_view>
#include <utility>

class osc::MwUiToolbar::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner,
        Widget* parent,
        std::string_view label,
        std::shared_ptr<MwUiSharedState> tab_state) :

        WidgetPrivate{owner, parent},
        state_{std::move(tab_state)}
    {
        set_name(label);
    }

    void on_draw()
    {
        if (begin_toolbar(name())) {
            draw_content();
        }
        ui::end_panel();
    }

private:
    void draw_content()
    {
        // document-related stuff
        draw_new_document_button();
        ui::same_line();
        draw_open_document_button();
        ui::same_line();
        draw_save_landmarks_button();
        ui::same_line();

        ui::draw_vertical_separator();
        ui::same_line();

        // undo/redo-related stuff
        undo_button_.on_draw();
        ui::same_line();
        redo_button_.on_draw();
        ui::same_line();

        ui::draw_vertical_separator();
        ui::same_line();

        // camera stuff
        draw_camera_lock_checkbox();
        ui::same_line();

        ui::draw_vertical_separator();
        ui::same_line();

        draw_visual_aids_menu_button();
        ui::same_line();

        draw_swap_source_destination_button();
        ui::same_line();
    }

    void draw_new_document_button()
    {
        if (ui::draw_button(MSMICONS_FILE)) {
            action_create_new_document(state_->upd_undoable());
        }
        ui::draw_tooltip_if_item_hovered(
            "Create New Document", "Creates the default scene (undoable)");
    }

    void draw_open_document_button()
    {
        ui::draw_button(MSMICONS_FOLDER_OPEN);
        if (ui::begin_popup_context_menu("##OpenFolder", ui::PopupFlag::MouseButtonLeft)) {
            if (ui::draw_menu_item("Load Source Mesh")) {
                action_prompt_user_to_load_mesh_file(
                    state_->get_undoable_shared_ptr(), MiDocumentInputIdentifier::Source);
            }
            if (ui::draw_menu_item("Load Destination Mesh")) {
                action_prompt_user_to_load_mesh_file(
                    state_->get_undoable_shared_ptr(), MiDocumentInputIdentifier::Destination);
            }
            ui::end_popup();
        }
        ui::draw_tooltip_if_item_hovered("Open File", "Open Source/Destination data");
    }

    void draw_save_landmarks_button()
    {
        if (ui::draw_button(MSMICONS_SAVE)) {
            action_prompt_user_to_save_paired_landmarks_to_csv(
                state_->get_scratch(), opyn::LandmarkCSVFlags::NoNames);
        }
        ui::draw_tooltip_if_item_hovered(
            "Save Landmarks to CSV (no names)",
            "Saves all pair-able landmarks to a CSV file, for external processing\n\n(legacy behavior: does not export names: use 'File' menu if you want the names)");
    }

    void draw_camera_lock_checkbox()
    {
        {
            bool linked = state_->is_cameras_linked();
            if (ui::draw_checkbox("link cameras", &linked)) {
                state_->set_cameras_linked(linked);
            }
        }

        ui::same_line();

        const bool disabled = not state_->is_cameras_linked();
        if (disabled) {
            ui::begin_disabled();
        }
        bool link_rotation = state_->is_only_camera_rotation_linked();
        if (ui::draw_checkbox("only link rotation", &link_rotation)) {
            state_->set_only_camera_rotation_linked(link_rotation);
        }
        if (disabled) {
            ui::end_disabled();
        }
    }

    void draw_visual_aids_menu_button()
    {
        if (ui::draw_button("visualization options " MSMICONS_COG)) {
            ui::open_popup("visualization_options_popup");
        }
        if (ui::begin_popup(
                "visualization_options_popup",
                {ui::PanelFlag::AlwaysAutoResize,
                 ui::PanelFlag::NoTitleBar,
                 ui::PanelFlag::NoSavedSettings})) {

            draw_rendering_options_editor(state_->upd_custom_rendering_options());
            draw_overlay_options_editor(state_->upd_overlay_decoration_options());
            {
                bool wireframe = state_->is_wireframe_mode_enabled();
                if (ui::draw_checkbox("Wireframe", &wireframe)) {
                    state_->set_wireframe_mode_enabled(wireframe);
                }
            }
            ui::end_popup();
        }
    }

    void draw_swap_source_destination_button()
    {
        if (ui::draw_button("swap source <-> destination")) {
            action_swap_source_destination(state_->upd_undoable());
        }
        ui::draw_tooltip_if_item_hovered(
            "Swap Source <-> Destination",
            "Swaps the source mesh with the destination mesh.\n\nNote: non-participating landmarks will be left in the source mesh, because they must always be there.");
    }

    std::shared_ptr<MwUiSharedState> state_;
    UndoButton undo_button_{&owner(), state_->get_undoable_shared_ptr(), MSMICONS_UNDO};
    RedoButton redo_button_{&owner(), state_->get_undoable_shared_ptr(), MSMICONS_REDO};
};

osc::MwUiToolbar::MwUiToolbar(
    Widget* parent, std::string_view label, std::shared_ptr<MwUiSharedState> shared_state) :
    Widget{std::make_unique<Impl>(*this, parent, label, std::move(shared_state))}
{}
void osc::MwUiToolbar::impl_on_draw()
{
    private_data().on_draw();
}

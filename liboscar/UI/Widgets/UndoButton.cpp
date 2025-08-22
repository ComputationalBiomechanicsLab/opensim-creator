#include "UndoButton.h"

#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/UndoRedo.h>

#include <cstddef>
#include <optional>

void osc::UndoButton::impl_on_draw()
{
    ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});

    bool was_disabled = false;
    if (not undo_redo_->can_undo()) {
        ui::begin_disabled();
        was_disabled = true;
    }
    if (ui::draw_button(button_icon_text_)) {
        undo_redo_->undo();
    }

    ui::same_line();

    ui::push_style_var(ui::StyleVar::FramePadding, {0.0f, ui::get_style_frame_padding().y});
    ui::draw_arrow_down_button("##UndoMenuOpener");
    ui::pop_style_var();

    if (was_disabled) {
        ui::end_disabled();
    }

    if (ui::begin_popup_context_menu("##OpenUndoMenu", ui::PopupFlag::MouseButtonLeft)) {
        int ui_id = 0;

        std::optional<size_t> desired_undo;
        for (size_t i = 0; i < undo_redo_->num_undo_entries(); ++i) {
            ui::push_id(ui_id++);
            if (ui::draw_selectable(undo_redo_->undo_entry_at(i).message())) {
                desired_undo = i;
            }
            ui::pop_id();
        }
        if (desired_undo) {
            undo_redo_->undo_to(*desired_undo);
        }
        ui::end_popup();
    }

    ui::pop_style_var();
}

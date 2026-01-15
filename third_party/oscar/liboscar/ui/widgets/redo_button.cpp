#include "redo_button.h"

#include <liboscar/ui/oscimgui.h>
#include <liboscar/utils/UndoRedo.h>

#include <cstddef>
#include <memory>
#include <optional>

void osc::RedoButton::impl_on_draw()
{
    ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});

    bool was_disabled = false;
    if (not undo_redo_->can_redo()) {
        ui::begin_disabled();
        was_disabled = true;
    }
    if (ui::draw_button(button_icon_text_)) {
        undo_redo_->redo();
    }

    ui::same_line();

    ui::push_style_var(ui::StyleVar::FramePadding, {0.0f, ui::get_style_frame_padding().y});
    ui::draw_arrow_down_button("##OpenRedoMenuToggler");
    ui::pop_style_var();

    if (was_disabled) {
        ui::end_disabled();
    }

    if (ui::begin_popup_context_menu("##OpenRedoMenu", ui::PopupFlag::MouseButtonLeft)) {
        int ui_id = 0;
        std::optional<size_t> desired_redo;
        for (size_t i = 0; i < undo_redo_->num_redo_entries(); ++i) {
            ui::push_id(ui_id++);
            if (ui::draw_selectable(undo_redo_->redo_entry_at(i).message())) {
                desired_redo = i;
            }
            ui::pop_id();
        }
        if (desired_redo) {
            undo_redo_->redo_to(*desired_redo);
        }
        ui::end_popup();
    }

    ui::pop_style_var();
}

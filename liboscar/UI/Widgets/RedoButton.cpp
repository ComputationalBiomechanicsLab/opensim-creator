#include "RedoButton.h"

#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/UndoRedo.h>

#include <memory>

osc::RedoButton::RedoButton(
    Widget* parent,
    std::shared_ptr<UndoRedoBase> undo_redo) :

    Widget{parent},
    undo_redo_{std::move(undo_redo)}
{}

void osc::RedoButton::impl_on_draw()
{
    ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});

    bool was_disabled = false;
    if (not undo_redo_->can_redo()) {
        ui::begin_disabled();
        was_disabled = true;
    }
    if (ui::draw_button(OSC_ICON_REDO)) {
        undo_redo_->redo();
    }

    ui::same_line();

    ui::push_style_var(ui::StyleVar::FramePadding, {0.0f, ui::get_style_frame_padding().y});
    ui::draw_button(OSC_ICON_CARET_DOWN);
    ui::pop_style_var();

    if (was_disabled) {
        ui::end_disabled();
    }

    if (ui::begin_popup_context_menu("##OpenRedoMenu", ui::PopupFlag::MouseButtonLeft)) {
        int ui_id = 0;
        for (size_t i = 0; i < undo_redo_->num_redo_entries(); ++i) {
            ui::push_id(ui_id++);
            if (ui::draw_selectable(undo_redo_->redo_entry_at(i).message())) {
                undo_redo_->redo_to(i);
            }
            ui::pop_id();
        }
        ui::end_popup();
    }

    ui::pop_style_var();
}

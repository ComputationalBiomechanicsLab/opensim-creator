#include "UndoButton.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/UndoRedo.h>

#include <IconsFontAwesome5.h>

#include <memory>

osc::UndoButton::UndoButton(std::shared_ptr<UndoRedoBase> undo_redo) :
    undo_redo_{std::move(undo_redo)}
{}

osc::UndoButton::~UndoButton() noexcept = default;

void osc::UndoButton::on_draw()
{
    int ui_id = 0;

    ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});

    bool was_disabled = false;
    if (not undo_redo_->can_undo()) {
        ui::begin_disabled();
        was_disabled = true;
    }
    if (ui::draw_button(ICON_FA_UNDO)) {
        undo_redo_->undo();
    }

    ui::same_line();

    ui::push_style_var(ui::StyleVar::FramePadding, {0.0f, ui::get_style_frame_padding().y});
    ui::draw_button(ICON_FA_CARET_DOWN);
    ui::pop_style_var();

    if (was_disabled) {
        ui::end_disabled();
    }

    if (ui::begin_popup_context_menu("##OpenUndoMenu", ui::PopupFlag::MouseButtonLeft)) {
        for (size_t i = 0; i < undo_redo_->num_undo_entries(); ++i) {
            ui::push_id(ui_id++);
            if (ui::draw_selectable(undo_redo_->undo_entry_at(i).message())) {
                undo_redo_->undo_to(i);
            }
            ui::pop_id();
        }
        ui::end_popup();
    }

    ui::pop_style_var();
}

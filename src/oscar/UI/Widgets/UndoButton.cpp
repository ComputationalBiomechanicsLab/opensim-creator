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
    int imgui_id = 0;

    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

    bool was_disabled = false;
    if (not undo_redo_->canUndo()) {
        ui::BeginDisabled();
        was_disabled = true;
    }
    if (ui::Button(ICON_FA_UNDO)) {
        undo_redo_->undo();
    }

    ui::SameLine();

    ui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ui::GetStyleFramePadding().y});
    ui::Button(ICON_FA_CARET_DOWN);
    ui::PopStyleVar();

    if (was_disabled) {
        ui::EndDisabled();
    }

    if (ui::BeginPopupContextItem("##OpenUndoMenu", ImGuiPopupFlags_MouseButtonLeft)) {
        for (ptrdiff_t i = 0; i < undo_redo_->getNumUndoEntriesi(); ++i) {
            ui::PushID(imgui_id++);
            if (ui::Selectable(undo_redo_->getUndoEntry(i).message())) {
                undo_redo_->undoTo(i);
            }
            ui::PopID();
        }
        ui::EndPopup();
    }

    ui::PopStyleVar();
}

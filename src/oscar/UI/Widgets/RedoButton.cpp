#include "RedoButton.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/UndoRedo.h>

#include <IconsFontAwesome5.h>

#include <memory>

osc::RedoButton::RedoButton(std::shared_ptr<UndoRedoBase> undo_redo) :
    undo_redo_{std::move(undo_redo)}
{}

osc::RedoButton::~RedoButton() noexcept = default;

void osc::RedoButton::onDraw()
{
    int imgui_id = 0;

    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

    bool was_disabled = false;
    if (not undo_redo_->canRedo()) {
        ui::BeginDisabled();
        was_disabled = true;
    }
    if (ui::Button(ICON_FA_REDO)) {
        undo_redo_->redo();
    }

    ui::SameLine();

    ui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ui::GetStyleFramePadding().y});
    ui::Button(ICON_FA_CARET_DOWN);
    ui::PopStyleVar();

    if (was_disabled) {
        ui::EndDisabled();
    }

    if (ui::BeginPopupContextItem("##OpenRedoMenu", ImGuiPopupFlags_MouseButtonLeft)) {
        for (ptrdiff_t i = 0; i < undo_redo_->getNumRedoEntriesi(); ++i) {
            ui::PushID(imgui_id++);
            if (ui::Selectable(undo_redo_->getRedoEntry(i).message())) {
                undo_redo_->redoTo(i);
            }
            ui::PopID();
        }
        ui::EndPopup();
    }

    ui::PopStyleVar();
}

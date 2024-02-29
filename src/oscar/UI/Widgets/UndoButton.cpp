#include "UndoButton.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/UndoRedo.h>

#include <IconsFontAwesome5.h>

#include <memory>

osc::UndoButton::UndoButton(std::shared_ptr<UndoRedoBase> undoRedo_) :
    m_UndoRedo{std::move(undoRedo_)}
{
}

osc::UndoButton::~UndoButton() noexcept = default;

void osc::UndoButton::onDraw()
{
    int imguiID = 0;

    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

    bool wasDisabled = false;
    if (!m_UndoRedo->canUndo())
    {
        ui::BeginDisabled();
        wasDisabled = true;
    }
    if (ui::Button(ICON_FA_UNDO))
    {
        m_UndoRedo->undo();
    }

    ui::SameLine();

    ui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ImGui::GetStyle().FramePadding.y});
    ui::Button(ICON_FA_CARET_DOWN);
    ui::PopStyleVar();

    if (wasDisabled)
    {
        ui::EndDisabled();
    }

    if (ui::BeginPopupContextItem("##OpenUndoMenu", ImGuiPopupFlags_MouseButtonLeft))
    {
        for (ptrdiff_t i = 0; i < m_UndoRedo->getNumUndoEntriesi(); ++i)
        {
            ui::PushID(imguiID++);
            if (ImGui::Selectable(m_UndoRedo->getUndoEntry(i).message().c_str()))
            {
                m_UndoRedo->undoTo(i);
            }
            ui::PopID();
        }
        ui::EndPopup();
    }

    ui::PopStyleVar();
}

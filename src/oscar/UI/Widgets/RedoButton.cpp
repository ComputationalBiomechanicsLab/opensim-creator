#include "RedoButton.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/UndoRedo.h>

#include <IconsFontAwesome5.h>

#include <memory>

osc::RedoButton::RedoButton(std::shared_ptr<UndoRedoBase> undoRedo_) :
    m_UndoRedo{std::move(undoRedo_)}
{
}

osc::RedoButton::~RedoButton() noexcept = default;

void osc::RedoButton::onDraw()
{
    int imguiID = 0;

    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

    bool wasDisabled = false;
    if (!m_UndoRedo->canRedo())
    {
        ui::BeginDisabled();
        wasDisabled = true;
    }
    if (ui::Button(ICON_FA_REDO))
    {
        m_UndoRedo->redo();
    }

    ui::SameLine();

    ui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ImGui::GetStyle().FramePadding.y});
    ui::Button(ICON_FA_CARET_DOWN);
    ui::PopStyleVar();

    if (wasDisabled)
    {
        ui::EndDisabled();
    }

    if (ImGui::BeginPopupContextItem("##OpenRedoMenu", ImGuiPopupFlags_MouseButtonLeft))
    {
        for (ptrdiff_t i = 0; i < m_UndoRedo->getNumRedoEntriesi(); ++i)
        {
            ui::PushID(imguiID++);
            if (ImGui::Selectable(m_UndoRedo->getRedoEntry(i).message().c_str()))
            {
                m_UndoRedo->redoTo(i);
            }
            ui::PopID();
        }
        ImGui::EndPopup();
    }

    ui::PopStyleVar();
}

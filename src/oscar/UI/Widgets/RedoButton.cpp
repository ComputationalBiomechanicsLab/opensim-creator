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

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

    bool wasDisabled = false;
    if (!m_UndoRedo->canRedo())
    {
        ImGui::BeginDisabled();
        wasDisabled = true;
    }
    if (ui::Button(ICON_FA_REDO))
    {
        m_UndoRedo->redo();
    }

    ui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ImGui::GetStyle().FramePadding.y});
    ui::Button(ICON_FA_CARET_DOWN);
    ImGui::PopStyleVar();

    if (wasDisabled)
    {
        ImGui::EndDisabled();
    }

    if (ImGui::BeginPopupContextItem("##OpenRedoMenu", ImGuiPopupFlags_MouseButtonLeft))
    {
        for (ptrdiff_t i = 0; i < m_UndoRedo->getNumRedoEntriesi(); ++i)
        {
            ImGui::PushID(imguiID++);
            if (ImGui::Selectable(m_UndoRedo->getRedoEntry(i).message().c_str()))
            {
                m_UndoRedo->redoTo(i);
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

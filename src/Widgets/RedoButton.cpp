#include "RedoButton.hpp"

#include "src/Utils/UndoRedo.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>

#include <memory>

osc::RedoButton::RedoButton(std::shared_ptr<UndoRedo> undoRedo_) :
    m_UndoRedo{std::move(undoRedo_)}
{
}

osc::RedoButton::~RedoButton() noexcept = default;

void osc::RedoButton::draw()
{
    int imguiID = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

    bool wasDisabled = false;
    if (!m_UndoRedo->canRedo())
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f * ImGui::GetStyle().Alpha);
        wasDisabled = true;
    }
    if (ImGui::Button(ICON_FA_REDO))
    {
        m_UndoRedo->redo();
    }

    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ImGui::GetStyle().FramePadding.y});
    ImGui::Button(ICON_FA_CARET_DOWN);
    ImGui::PopStyleVar();

    if (wasDisabled)
    {
        ImGui::PopStyleVar();
    }

    if (ImGui::BeginPopupContextItem("##OpenRedoMenu", ImGuiPopupFlags_MouseButtonLeft))
    {
        for (ptrdiff_t i = 0; i < m_UndoRedo->getNumRedoEntriesi(); ++i)
        {
            ImGui::PushID(imguiID++);
            if (ImGui::Selectable(m_UndoRedo->getRedoEntry(i).getMessage().c_str()))
            {
                m_UndoRedo->redoTo(i);
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

#include "UndoButton.hpp"

#include "src/Utils/UndoRedo.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>

#include <memory>

osc::UndoButton::UndoButton(std::shared_ptr<UndoRedo> undoRedo_) :
    m_UndoRedo{std::move(undoRedo_)}
{
}
osc::UndoButton::UndoButton(UndoButton&& tmp) noexcept = default;
osc::UndoButton& osc::UndoButton::operator=(UndoButton&&) noexcept = default;
osc::UndoButton::~UndoButton() noexcept = default;

void osc::UndoButton::draw()
{
    int imguiID = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

    bool wasDisabled = false;
    if (!m_UndoRedo->canUndo())
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f * ImGui::GetStyle().Alpha);
        wasDisabled = true;
    }
    if (ImGui::Button(ICON_FA_UNDO))
    {
        m_UndoRedo->undo();
    }

    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, ImGui::GetStyle().FramePadding.y});
    ImGui::Button(ICON_FA_CARET_DOWN);
    ImGui::PopStyleVar();

    if (wasDisabled)
    {
        ImGui::PopStyleVar();
    }

    if (ImGui::BeginPopupContextItem("##OpenUndoMenu", ImGuiPopupFlags_MouseButtonLeft))
    {
        for (size_t i = 0; i < m_UndoRedo->getNumUndoEntries(); ++i)
        {
            ImGui::PushID(imguiID++);
            if (ImGui::Selectable(m_UndoRedo->getUndoEntry(i).getMessage().c_str()))
            {
                m_UndoRedo->undoTo(i);
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

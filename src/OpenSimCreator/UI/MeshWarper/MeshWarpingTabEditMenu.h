#pragma once

#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <imgui.h>

#include <memory>
#include <utility>

namespace osc
{
    // widget: the 'edit' menu (a sub menu of the main menu)
    class MeshWarpingTabEditMenu final {
    public:
        explicit MeshWarpingTabEditMenu(
            std::shared_ptr<MeshWarpingTabSharedState> tabState_) :
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (ImGui::BeginMenu("Edit"))
            {
                drawContent();
                ImGui::EndMenu();
            }
        }

    private:

        void drawContent()
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", nullptr, m_State->canUndo()))
            {
                m_State->undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", nullptr, m_State->canRedo()))
            {
                m_State->redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A"))
            {
                m_State->selectAll();
            }
            if (ImGui::MenuItem("Deselect", "Escape", nullptr, m_State->hasSelection()))
            {
                m_State->clearSelection();
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

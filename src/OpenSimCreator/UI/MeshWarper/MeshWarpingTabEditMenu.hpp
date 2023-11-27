#pragma once

#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>

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
            if (ImGui::MenuItem("Undo", nullptr, nullptr, m_State->editedDocument->canUndo()))
            {
                ActionUndo(*m_State->editedDocument);
            }
            if (ImGui::MenuItem("Redo", nullptr, nullptr, m_State->editedDocument->canRedo()))
            {
                ActionRedo(*m_State->editedDocument);
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

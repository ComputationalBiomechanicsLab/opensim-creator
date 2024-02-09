#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <IconsFontAwesome5.h>
#include <imgui.h>

#include <memory>
#include <utility>

namespace osc
{
    // widget: the 'edit' menu (a sub menu of the main menu)
    class MeshWarpingTabActionsMenu final {
    public:
        explicit MeshWarpingTabActionsMenu(
            std::shared_ptr<MeshWarpingTabSharedState> tabState_) :
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (ImGui::BeginMenu("Actions"))
            {
                drawContent();
                ImGui::EndMenu();
            }
        }

    private:

        void drawContent()
        {
            drawClearLandmarksMenuItem();
            drawClearNonParticipatingLandmarksMenuItem();
        }

        void drawClearLandmarksMenuItem()
        {
            bool const hasLandmarks = ContainsLandmarks(m_State->getScratch());

            if (!hasLandmarks)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::MenuItem(ICON_FA_ERASER " clear landmarks"))
            {
                ActionClearAllLandmarks(m_State->updUndoable());
            }

            if (!hasLandmarks)
            {
                ImGui::EndDisabled();
            }
        }

        void drawClearNonParticipatingLandmarksMenuItem()
        {
            bool const hasNonParticipatingLandmarks =
                ContainsNonParticipatingLandmarks(m_State->getScratch());

            if (!hasNonParticipatingLandmarks)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::MenuItem(ICON_FA_ERASER " clear non-participating landmarks"))
            {
                ActionClearAllNonParticipatingLandmarks(m_State->updUndoable());
            }

            if (!hasNonParticipatingLandmarks)
            {
                ImGui::EndDisabled();
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

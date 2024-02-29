#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <IconsFontAwesome5.h>
#include <oscar/UI/oscimgui.h>

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
            if (ui::BeginMenu("Actions"))
            {
                drawContent();
                ui::EndMenu();
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
                ui::BeginDisabled();
            }

            if (ui::MenuItem(ICON_FA_ERASER " clear landmarks"))
            {
                ActionClearAllLandmarks(m_State->updUndoable());
            }

            if (!hasLandmarks)
            {
                ui::EndDisabled();
            }
        }

        void drawClearNonParticipatingLandmarksMenuItem()
        {
            bool const hasNonParticipatingLandmarks =
                ContainsNonParticipatingLandmarks(m_State->getScratch());

            if (!hasNonParticipatingLandmarks)
            {
                ui::BeginDisabled();
            }

            if (ui::MenuItem(ICON_FA_ERASER " clear non-participating landmarks"))
            {
                ActionClearAllNonParticipatingLandmarks(m_State->updUndoable());
            }

            if (!hasNonParticipatingLandmarks)
            {
                ui::EndDisabled();
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

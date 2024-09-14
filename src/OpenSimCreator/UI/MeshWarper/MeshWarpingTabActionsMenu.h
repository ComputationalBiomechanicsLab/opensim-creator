#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <oscar/Platform/IconCodepoints.h>
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
            if (ui::begin_menu("Actions"))
            {
                drawContent();
                ui::end_menu();
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
            const bool hasLandmarks = ContainsLandmarks(m_State->getScratch());

            if (!hasLandmarks)
            {
                ui::begin_disabled();
            }

            if (ui::draw_menu_item(OSC_ICON_ERASER " clear landmarks"))
            {
                ActionClearAllLandmarks(m_State->updUndoable());
            }

            if (!hasLandmarks)
            {
                ui::end_disabled();
            }
        }

        void drawClearNonParticipatingLandmarksMenuItem()
        {
            const bool hasNonParticipatingLandmarks =
                ContainsNonParticipatingLandmarks(m_State->getScratch());

            if (!hasNonParticipatingLandmarks)
            {
                ui::begin_disabled();
            }

            if (ui::draw_menu_item(OSC_ICON_ERASER " clear non-participating landmarks"))
            {
                ActionClearAllNonParticipatingLandmarks(m_State->updUndoable());
            }

            if (!hasNonParticipatingLandmarks)
            {
                ui::end_disabled();
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

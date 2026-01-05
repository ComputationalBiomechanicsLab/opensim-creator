#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <libopensimcreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <libopensimcreator/Platform/msmicons.h>

#include <liboscar/UI/oscimgui.h>

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

            if (not hasLandmarks) {
                ui::begin_disabled();
            }
            if (ui::draw_menu_item(MSMICONS_ERASER " clear landmarks")) {
                ActionClearAllLandmarks(m_State->updUndoable());
            }
            if (not hasLandmarks) {
                ui::end_disabled();
            }
        }

        void drawClearNonParticipatingLandmarksMenuItem()
        {
            const bool hasNonParticipatingLandmarks =
                ContainsNonParticipatingLandmarks(m_State->getScratch());

            if (not hasNonParticipatingLandmarks) {
                ui::begin_disabled();
            }
            if (ui::draw_menu_item(MSMICONS_ERASER " clear non-participating landmarks")) {
                ActionClearAllNonParticipatingLandmarks(m_State->updUndoable());
            }
            if (not hasNonParticipatingLandmarks) {
                ui::end_disabled();
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

#pragma once

#include <libopensimcreator/documents/mesh_warper/tps_document_helpers.h>
#include <libopensimcreator/documents/mesh_warper/undoable_tps_document_actions.h>
#include <libopensimcreator/ui/mesh_warper/mesh_warping_tab_shared_state.h>
#include <libopensimcreator/platform/msmicons.h>

#include <liboscar/ui/oscimgui.h>

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

#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_helpers.h>
#include <libopensimcreator/documents/mesh_warper/mw_undoable_actions.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>
#include <libopensimcreator/platform/msmicons.h>

#include <liboscar/ui/oscimgui.h>

#include <memory>
#include <utility>

namespace osc
{
    // widget: the 'edit' menu (a sub menu of the main menu)
    class MwUiActionsMenu final {
    public:
        explicit MwUiActionsMenu(std::shared_ptr<MwUiSharedState> tab_state) :
            state_{std::move(tab_state)}
        {}

        void on_draw()
        {
            if (ui::begin_menu("Actions")) {
                draw_content();
                ui::end_menu();
            }
        }

    private:
        void draw_content()
        {
            draw_clear_landmarks_menu_item();
            draw_clear_non_participating_landmarks_menu_item();
        }

        void draw_clear_landmarks_menu_item()
        {
            const bool has_landmarks = contains_landmarks(state_->get_scratch());

            if (not has_landmarks) {
                ui::begin_disabled();
            }
            if (ui::draw_menu_item(MSMICONS_ERASER " clear landmarks")) {
                action_clear_all_landmarks(state_->upd_undoable());
            }
            if (not has_landmarks) {
                ui::end_disabled();
            }
        }

        void draw_clear_non_participating_landmarks_menu_item()
        {
            const bool has_non_participating_landmarks =
                contains_non_participating_landmarks(state_->get_scratch());

            if (not has_non_participating_landmarks) {
                ui::begin_disabled();
            }
            if (ui::draw_menu_item(MSMICONS_ERASER " clear non-participating landmarks")) {
                action_clear_all_non_participating_landmarks(state_->upd_undoable());
            }
            if (not has_non_participating_landmarks) {
                ui::end_disabled();
            }
        }

        std::shared_ptr<MwUiSharedState> state_;
    };
}

#pragma once

#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>

#include <liboscar/ui/oscimgui.h>

#include <memory>
#include <utility>

namespace osc
{
    // widget: the 'edit' menu (a sub menu of the main menu)
    class MwUiEditMenu final {
    public:
        explicit MwUiEditMenu(std::shared_ptr<MwUiSharedState> tab_state) :
            state_{std::move(tab_state)}
        {}

        void on_draw()
        {
            if (ui::begin_menu("Edit")) {
                draw_content();
                ui::end_menu();
            }
        }

    private:
        void draw_content()
        {
            if (ui::draw_menu_item(
                    "Undo", ModifierKey::Ctrl | Key::Z, nullptr, state_->can_undo())) {
                state_->undo();
            }
            if (ui::draw_menu_item(
                    "Redo",
                    ModifierKey::Ctrl | ModifierKey::Shift | Key::Z,
                    nullptr,
                    state_->can_redo())) {
                state_->redo();
            }
            ui::draw_separator();
            if (ui::draw_menu_item("Select All", ModifierKey::Ctrl | Key::A)) {
                state_->select_all();
            }
            if (ui::draw_menu_item("Deselect", Key::Escape, nullptr, state_->has_selection())) {
                state_->clear_selection();
            }
        }

        std::shared_ptr<MwUiSharedState> state_;
    };
}

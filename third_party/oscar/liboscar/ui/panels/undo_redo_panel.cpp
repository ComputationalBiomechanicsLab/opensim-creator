#include "undo_redo_panel.h"

#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utils/UndoRedo.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

using namespace osc;

class osc::UndoRedoPanel::Impl final : public PanelPrivate {
public:
    Impl(
        UndoRedoPanel& owner,
        Widget* parent,
        std::string_view panel_name,
        std::shared_ptr<UndoRedoBase> storage) :

        PanelPrivate{owner, parent, panel_name},
        storage_{std::move(storage)}
    {}

    void draw_content()
    {
        UndoRedoPanel::draw_content(*storage_);
    }

private:
    std::shared_ptr<UndoRedoBase> storage_;
};


void osc::UndoRedoPanel::draw_content(UndoRedoBase& storage)
{
    if (ui::draw_button("undo")) {
        storage.undo();
    }

    ui::same_line();

    if (ui::draw_button("redo")) {
        storage.redo();
    }

    int ui_id = 0;

    // draw undo entries oldest (highest index) to newest (lowest index)
    std::optional<ptrdiff_t> user_enacted_undo;
    for (auto i = static_cast<ptrdiff_t>(storage.num_undo_entries())-1; i >= 0; --i) {
        ui::push_id(ui_id++);
        if (ui::draw_selectable(storage.undo_entry_at(i).message())) {
            user_enacted_undo = i;
        }
        ui::pop_id();
    }
    if (user_enacted_undo) {
        storage.undo_to(*user_enacted_undo);
    }

    ui::push_id(ui_id++);
    ui::draw_text("  %s", storage.head().message().c_str());
    ui::pop_id();

    // draw redo entries oldest (lowest index) to newest (highest index)
    std::optional<ptrdiff_t> user_enacted_redo;
    for (size_t i = 0; i < storage.num_redo_entries(); ++i) {
        ui::push_id(ui_id++);
        if (ui::draw_selectable(storage.redo_entry_at(i).message())) {
            user_enacted_redo = i;
        }
        ui::pop_id();
    }
    if (user_enacted_redo) {
        storage.redo_to(*user_enacted_redo);
    }
}

osc::UndoRedoPanel::UndoRedoPanel(Widget* parent, std::string_view panel_name, std::shared_ptr<UndoRedoBase> storage) :
    Panel{std::make_unique<Impl>(*this, parent, panel_name, std::move(storage))}
{}
void osc::UndoRedoPanel::impl_draw_content() { private_data().draw_content(); }

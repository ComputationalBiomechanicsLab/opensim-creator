#include "UndoRedoPanel.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/UndoRedo.h>

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

class osc::UndoRedoPanel::Impl final : public StandardPanelImpl {
public:
    Impl(
        std::string_view panel_name,
        std::shared_ptr<UndoRedoBase> storage) :

        StandardPanelImpl{panel_name},
        storage_{std::move(storage)}
    {}

private:
    void impl_draw_content() final
    {
        UndoRedoPanel::draw_content(*storage_);
    }

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
    for (ptrdiff_t i = storage.num_undo_entriesi()-1; 0 <= i and i < storage.num_undo_entriesi(); --i) {
        ui::push_id(ui_id++);
        if (ui::draw_selectable(storage.undo_entry_at(i).message())) {
            storage.undo_to(i);
        }
        ui::pop_id();
    }

    ui::push_id(ui_id++);
    ui::draw_text("  %s", storage.head().message().c_str());
    ui::pop_id();

    // draw redo entries oldest (lowest index) to newest (highest index)
    for (ptrdiff_t i = 0; i < storage.num_redo_entriesi(); ++i) {
        ui::push_id(ui_id++);
        if (ui::draw_selectable(storage.redo_entry_at(i).message())) {
            storage.redo_to(i);
        }
        ui::pop_id();
    }
}

osc::UndoRedoPanel::UndoRedoPanel(std::string_view panel_name, std::shared_ptr<UndoRedoBase> storage) :
    impl_{std::make_unique<Impl>(panel_name, std::move(storage))}
{}

osc::UndoRedoPanel::UndoRedoPanel(UndoRedoPanel&&) noexcept = default;
osc::UndoRedoPanel& osc::UndoRedoPanel::operator=(UndoRedoPanel&&) noexcept = default;
osc::UndoRedoPanel::~UndoRedoPanel() noexcept = default;

CStringView osc::UndoRedoPanel::impl_get_name() const
{
    return impl_->name();
}

bool osc::UndoRedoPanel::impl_is_open() const
{
    return impl_->is_open();
}

void osc::UndoRedoPanel::impl_open()
{
    return impl_->open();
}

void osc::UndoRedoPanel::impl_close()
{
    return impl_->close();
}

void osc::UndoRedoPanel::impl_on_draw()
{
    return impl_->on_draw();
}

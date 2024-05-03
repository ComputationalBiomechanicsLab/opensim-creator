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
    if (ui::Button("undo")) {
        storage.undo();
    }

    ui::SameLine();

    if (ui::Button("redo")) {
        storage.redo();
    }

    int ui_id = 0;

    // draw undo entries oldest (highest index) to newest (lowest index)
    for (ptrdiff_t i = storage.getNumUndoEntriesi()-1; 0 <= i and i < storage.getNumUndoEntriesi(); --i) {
        ui::PushID(ui_id++);
        if (ui::Selectable(storage.getUndoEntry(i).message())) {
            storage.undoTo(i);
        }
        ui::PopID();
    }

    ui::PushID(ui_id++);
    ui::Text("  %s", storage.getHead().message().c_str());
    ui::PopID();

    // draw redo entries oldest (lowest index) to newest (highest index)
    for (ptrdiff_t i = 0; i < storage.getNumRedoEntriesi(); ++i) {
        ui::PushID(ui_id++);
        if (ui::Selectable(storage.getRedoEntry(i).message())) {
            storage.redoTo(i);
        }
        ui::PopID();
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

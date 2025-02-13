#pragma once

#include <liboscar/Platform/Widget.h>

#include <memory>

namespace osc { class UndoRedoBase; }

namespace osc
{
    // a user-visible button, with a history dropdown menu, that performs an undo operation
    class UndoButton final : public Widget {
    public:
        explicit UndoButton(Widget* parent, std::shared_ptr<UndoRedoBase>);

    private:
        void impl_on_draw() final;

        std::shared_ptr<UndoRedoBase> undo_redo_;
    };
}

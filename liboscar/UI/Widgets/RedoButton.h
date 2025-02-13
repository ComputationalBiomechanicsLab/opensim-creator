#pragma once

#include <liboscar/Platform/Widget.h>

#include <memory>

namespace osc { class UndoRedoBase; }

namespace osc
{
    class RedoButton final : public Widget {
    public:
        explicit RedoButton(Widget* parent, std::shared_ptr<UndoRedoBase>);

    private:
        void impl_on_draw();

        std::shared_ptr<UndoRedoBase> undo_redo_;
    };
}

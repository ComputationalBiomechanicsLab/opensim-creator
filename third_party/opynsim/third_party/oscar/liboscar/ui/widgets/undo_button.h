#pragma once

#include <liboscar/platform/widget.h>

#include <memory>
#include <string>
#include <utility>

namespace osc { class UndoRedoBase; }

namespace osc
{
    // a user-visible button, with a history dropdown menu, that performs an undo operation
    class UndoButton final : public Widget {
    public:
        explicit UndoButton(
            Widget* parent,
            std::shared_ptr<UndoRedoBase> undo_redo,
            std::string button_icon_text = "<") :

            Widget{parent},
            undo_redo_{std::move(undo_redo)},
            button_icon_text_{std::move(button_icon_text)}
        {}

    private:
        void impl_on_draw() final;

        std::shared_ptr<UndoRedoBase> undo_redo_;
        std::string button_icon_text_;
    };
}

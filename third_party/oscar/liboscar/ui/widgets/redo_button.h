#pragma once

#include <liboscar/platform/widget.h>

#include <memory>
#include <string>
#include <utility>

namespace osc { class UndoRedoBase; }

namespace osc
{
    class RedoButton final : public Widget {
    public:
        explicit RedoButton(
            Widget* parent,
            std::shared_ptr<UndoRedoBase> undo_redo,
            std::string button_icon_text = ">") :

            Widget{parent},
            undo_redo_{std::move(undo_redo)},
            button_icon_text_{std::move(button_icon_text)}
        {}

    private:
        void impl_on_draw();

        std::shared_ptr<UndoRedoBase> undo_redo_;
        std::string button_icon_text_;
    };
}

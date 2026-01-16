#pragma once

#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <string_view>

namespace osc { class UndoRedoBase; }

namespace osc
{
    // a user-visible panel that lists undo/redo history
    class UndoRedoPanel final : public Panel {
    public:
        static void draw_content(UndoRedoBase&);

        explicit UndoRedoPanel(
            Widget* parent,
            std::string_view panel_name,
            std::shared_ptr<UndoRedoBase>
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

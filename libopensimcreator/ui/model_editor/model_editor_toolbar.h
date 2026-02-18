#pragma once

#include <liboscar/platform/widget.h>

#include <memory>
#include <string_view>

namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorToolbar final : public Widget {
    public:
        explicit ModelEditorToolbar(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<UndoableModelStatePair>
        );
    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

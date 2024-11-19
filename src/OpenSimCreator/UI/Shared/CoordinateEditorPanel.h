#pragma once

#include <oscar/UI/Panels/Panel.h>

#include <memory>
#include <string_view>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class CoordinateEditorPanel final : public Panel {
    public:
        explicit CoordinateEditorPanel(
            std::string_view panelName,
            Widget& parent,
            std::shared_ptr<IModelStatePair>
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

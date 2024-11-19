#pragma once

#include <oscar/UI/Panels/Panel.h>

#include <string_view>
#include <memory>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class PropertiesPanel final : public Panel {
    public:
        explicit PropertiesPanel(
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

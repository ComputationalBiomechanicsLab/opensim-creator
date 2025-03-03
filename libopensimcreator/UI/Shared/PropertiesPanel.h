#pragma once

#include <liboscar/UI/Panels/Panel.h>

#include <string_view>
#include <memory>

namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class PropertiesPanel final : public Panel {
    public:
        explicit PropertiesPanel(
            Widget* parent,
            std::string_view panelName,
            std::shared_ptr<IModelStatePair>
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

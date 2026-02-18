#pragma once

#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <string_view>

namespace opyn { class ModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class PropertiesPanel final : public Panel {
    public:
        explicit PropertiesPanel(
            Widget* parent,
            std::string_view panelName,
            std::shared_ptr<opyn::ModelStatePair>
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

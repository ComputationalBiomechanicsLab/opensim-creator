#pragma once

#include <liboscar/ui/panels/Panel.h>

#include <memory>
#include <string_view>

namespace osc { class Environment; }
namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class OutputPlotsPanel final : public Panel {
    public:
        explicit OutputPlotsPanel(
            Widget* parent,
            std::string_view panelName,
            std::shared_ptr<Environment>,
            ISimulatorUIAPI*
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

#pragma once

#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <string_view>

namespace osc { class ModelStatePair; }

namespace osc
{
    class OutputWatchesPanel final : public Panel {
    public:
        explicit OutputWatchesPanel(
            Widget* parent,
            std::string_view panelName,
            std::shared_ptr<const ModelStatePair>
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
